/*
  Copyright (c) 2012 Carsten Burstedde, Donna Calhoun
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "swirl_user.h"

#include <fclaw_package.h>

#include <fclaw2d_forestclaw.h>
#include <fclaw2d_options.h>
#include <fclaw2d_domain.h>
#include <fclaw2d_diagnostics.h>
#include <fclaw2d_convenience.h>
#include <fclaw2d_global.h>
#include <fclaw2d_vtable.h>

#include <fclaw2d_clawpatch_options.h>
#include <fclaw2d_clawpatch.h>

#include <fc2d_clawpack46_options.h>
#include <fc2d_clawpack5_options.h>

#include <fc2d_clawpack46.h>
#include <fc2d_clawpack5.h>

static int s_user_options_package_id = -1;

static void *
swirl_register (user_options_t *user, sc_options_t * opt)
{
    /* [user] User options */
    sc_options_add_double (opt, 0, "period", &user->period, 4,
                           "Period of the flow field [4]");

    sc_options_add_int (opt, 0, "claw-version", &user->claw_version, 5,
                           "Clawpack_version (4 or 5) [5]");

    user->is_registered = 1;

    return NULL;
}

static fclaw_exit_type_t
swirl_postprocess(user_options_t *user)
{
    /* nothing to post-process yet ... */
    return FCLAW_NOEXIT;
}


static fclaw_exit_type_t
swirl_check (user_options_t *user)
{
    /* Nothing to check ? */
    return FCLAW_NOEXIT;
}

static fclaw_exit_type_t
swirl_destroy(user_options_t *user)
{
    /* Nothing to destroy */
    return FCLAW_NOEXIT;
}

/* ------- Generic option handling routines that call above routines ----- */
static void*
options_register (fclaw_app_t * app, void *package, sc_options_t * opt)
{
    user_options_t *user;

    FCLAW_ASSERT (app != NULL);
    FCLAW_ASSERT (package != NULL);
    FCLAW_ASSERT (opt != NULL);

    user = (user_options_t*) package;

    return swirl_register(user,opt);
}

static fclaw_exit_type_t
options_postprocess (fclaw_app_t * a, void *package, void *registered)
{
    FCLAW_ASSERT (a != NULL);
    FCLAW_ASSERT (package != NULL);
    FCLAW_ASSERT (registered == NULL);

    /* errors from the key-value options would have showed up in parsing */
    user_options_t *user = (user_options_t *) package;

    /* post-process this package */
    FCLAW_ASSERT(user->is_registered);

    /* Convert strings to arrays */
    return swirl_postprocess (user);
}


static fclaw_exit_type_t
options_check(fclaw_app_t *app, void *package,void *registered)
{
    user_options_t           *user;

    FCLAW_ASSERT (app != NULL);
    FCLAW_ASSERT (package != NULL);
    FCLAW_ASSERT(registered == NULL);

    user = (user_options_t*) package;

    return swirl_check(user);
}

static void
options_destroy (fclaw_app_t * app, void *package, void *registered)
{
    user_options_t *user;

    FCLAW_ASSERT (app != NULL);
    FCLAW_ASSERT (package != NULL);
    FCLAW_ASSERT (registered == NULL);

    user = (user_options_t*) package;
    FCLAW_ASSERT (user->is_registered);

    swirl_destroy (user);

    FCLAW_FREE (user);
}


static const fclaw_app_options_vtable_t options_vtable_user =
{
    options_register,
    options_postprocess,
    options_check,
    options_destroy
};

/* ------------- User options access functions --------------------- */

static
user_options_t* swirl_options_register (fclaw_app_t * app,
                                       const char *configfile)
{
    user_options_t *user;
    FCLAW_ASSERT (app != NULL);

    user = FCLAW_ALLOC (user_options_t, 1);
    fclaw_app_options_register (app,"user", configfile, &options_vtable_user,
                                user);

    fclaw_app_set_attribute(app,"user",user);
    return user;
}

static 
void user_options_store (fclaw2d_global_t* glob, user_options_t* user)
{
    FCLAW_ASSERT(s_user_options_package_id == -1);
    int id = fclaw_package_container_add_pkg(glob,user);
    s_user_options_package_id = id;
}

const user_options_t* swirl_get_options(fclaw2d_global_t* glob)
{
    int id = s_user_options_package_id;
    return (user_options_t*) fclaw_package_get_options(glob, id);    
}
/* ------------------------- ... and here ---------------------------- */



static
fclaw2d_domain_t* create_domain(sc_MPI_Comm mpicomm, fclaw_options_t* gparms)
{
    /* Mapped, multi-block domain */
    p4est_connectivity_t     *conn = NULL;
    fclaw2d_domain_t         *domain;
    fclaw2d_map_context_t    *cont = NULL;

    /* Map unit square to disk using mapc2m_disk.f */
    gparms->manifold = 0;
    conn = p4est_connectivity_new_unitsquare();
    cont = fclaw2d_map_new_nomap();

    domain = fclaw2d_domain_new_conn_map (mpicomm, gparms->minlevel, conn, cont);
    fclaw2d_domain_list_levels(domain, FCLAW_VERBOSITY_ESSENTIAL);
    fclaw2d_domain_list_neighbors(domain, FCLAW_VERBOSITY_DEBUG);  
    return domain;
}

static
void run_program(fclaw2d_global_t* glob)
{
    user_options_t           *user;

    /* ---------------------------------------------------------------
       Set domain data.
       --------------------------------------------------------------- */
    fclaw2d_domain_data_new(glob->domain);

    user = (user_options_t*) swirl_get_options(glob);

    /* Initialize virtual table for ForestClaw */
    fclaw2d_vtable_initialize();
    fclaw2d_diagnostics_vtable_initialize();

    /* Initialize virtual tables for solvers */
    if (user->claw_version == 4)
    {
        fc2d_clawpack46_vtable_initialize();
    }
    else if (user->claw_version == 5)
    {
        fc2d_clawpack5_vtable_initialize();
    }

    swirl_link_solvers(glob);

    /* ---------------------------------------------------------------
       Run
       --------------------------------------------------------------- */
    fclaw2d_initialize(glob);
    fclaw2d_run(glob);
    fclaw2d_finalize(glob);
}

int
main (int argc, char **argv)
{
    fclaw_app_t *app;
    int first_arg;
    fclaw_exit_type_t vexit;

    /* Options */
    sc_options_t                *options;
    user_options_t              *user;
    fclaw_options_t             *gparms;
    fclaw2d_clawpatch_options_t *clawpatchopt;
    fc2d_clawpack46_options_t   *claw46opt;
    fc2d_clawpack5_options_t    *claw5opt;

    fclaw2d_global_t         *glob;
    fclaw2d_domain_t         *domain;
    sc_MPI_Comm mpicomm;

    int retval;

    /* Initialize application */
    app = fclaw_app_new (&argc, &argv, NULL);

    /* Create new options packages */
    gparms =                     fclaw_options_register(app,"fclaw_options.ini");
    clawpatchopt =   fclaw2d_clawpatch_options_register(app,"fclaw_options.ini");
    claw46opt =        fc2d_clawpack46_options_register(app,"fclaw_options.ini");
    claw5opt =          fc2d_clawpack5_options_register(app,"fclaw_options.ini");
    user =                        swirl_options_register(app,"fclaw_options.ini");  

    /* Read configuration file(s) and command line, and process options */
    options = fclaw_app_get_options (app);
    retval = fclaw_options_read_from_file(options);
    vexit =  fclaw_app_options_parse (app, &first_arg,"fclaw_options.ini.used");

    /* at this point gparms is valid */
    mpicomm = fclaw_app_get_mpi_size_rank (app, NULL, NULL);
    domain = create_domain(mpicomm, gparms);
    
    /* Create global structure which stores the domain, timers, etc */
    glob = fclaw2d_global_new();
    fclaw2d_global_store_domain(glob, domain);

    /* Store option packages in glob */
    fclaw2d_options_store (glob, gparms);
    fclaw2d_clawpatch_options_store (glob, clawpatchopt);
    fc2d_clawpack46_options_store (glob, claw46opt);
    fc2d_clawpack5_options_store (glob, claw5opt);
    user_options_store (glob, user);

    /* Run the program */
    if (!retval & !vexit)
    {
        run_program(glob);
    }
    
    fclaw2d_global_destroy(glob);
    fclaw_app_destroy (app);

    return 0;
}
