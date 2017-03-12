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

#include <fclaw2d_forestclaw.h>
#include <forestclaw2d.h>
#include <p4est_base.h>

#include <fclaw2d_patch.h>
#include <fclaw2d_domain.h>

static
fclaw2d_patch_vtable_t* patch_vt()
{
    static fclaw2d_patch_vtable_t s_patch_vt;
    return &s_patch_vt;
}

fclaw2d_patch_vtable_t* fclaw2d_patch_vt()
{
    return patch_vt();
}

struct fclaw2d_patch_data
{
    fclaw2d_patch_relation_t face_neighbors[4];
    fclaw2d_patch_relation_t corner_neighbors[4];
    int corners[4];
    int block_corner_count[4];
    int on_coarsefine_interface;
    int has_finegrid_neighbors;
    int neighbors_set;

    void *user_patch; /* Start of attempt to "virtualize" the user patch. */
};

fclaw2d_patch_data_t*
fclaw2d_patch_get_user_data(fclaw2d_patch_t* patch)
{
    return (fclaw2d_patch_data_t *) patch->user;
}

void*
fclaw2d_patch_get_user_patch(fclaw2d_patch_t* patch)

{
    fclaw2d_patch_data_t *pdata = fclaw2d_patch_get_user_data(patch);
    FCLAW_ASSERT(pdata != NULL);
    return pdata->user_patch;
}


void fclaw2d_patch_set_face_type(fclaw2d_patch_t *patch,int iface,
                                 fclaw2d_patch_relation_t face_type)
{
    fclaw2d_patch_data_t *pdata = fclaw2d_patch_get_user_data(patch);
    pdata->face_neighbors[iface] = face_type;
}

void fclaw2d_patch_set_corner_type(fclaw2d_patch_t *patch,int icorner,
                                   fclaw2d_patch_relation_t corner_type)
{
    fclaw2d_patch_data_t *pdata = fclaw2d_patch_get_user_data(patch);
    pdata->corner_neighbors[icorner] = corner_type;
    pdata->corners[icorner] = 1;
}

void fclaw2d_patch_set_missing_corner(fclaw2d_patch_t *patch,int icorner)
{
    fclaw2d_patch_data_t *pdata = fclaw2d_patch_get_user_data(patch);
    pdata->corners[icorner] = 0;
}

fclaw2d_patch_relation_t fclaw2d_patch_get_face_type(fclaw2d_patch_t* patch,
                                                     int iface)
{
    fclaw2d_patch_data_t *pdata = fclaw2d_patch_get_user_data(patch);
    FCLAW_ASSERT(pdata->neighbors_set != 0);
    FCLAW_ASSERT(0 <= iface && iface < 4);
    return pdata->face_neighbors[iface];
}

fclaw2d_patch_relation_t fclaw2d_patch_get_corner_type(fclaw2d_patch_t* patch,
                                                       int icorner)
{
    fclaw2d_patch_data_t *pdata = fclaw2d_patch_get_user_data(patch);
    FCLAW_ASSERT(pdata->corners[icorner] != 0);
    FCLAW_ASSERT(pdata->neighbors_set != 0);
    return pdata->corner_neighbors[icorner];
}

int fclaw2d_patch_corner_is_missing(fclaw2d_patch_t* patch,
                                    int icorner)
{
    fclaw2d_patch_data_t *pdata = fclaw2d_patch_get_user_data(patch);
    return !pdata->corners[icorner];
}

void fclaw2d_patch_neighbors_set(fclaw2d_patch_t* patch)
{
    int iface, icorner;
    fclaw2d_patch_data_t *pdata = fclaw2d_patch_get_user_data(patch);
    FCLAW_ASSERT(pdata->neighbors_set == 0);

    pdata->has_finegrid_neighbors = 0;
    pdata->on_coarsefine_interface = 0;
    for (iface = 0; iface < 4; iface++)
    {
        fclaw2d_patch_relation_t nt;
        nt = pdata->face_neighbors[iface];
        if (nt == FCLAW2D_PATCH_HALFSIZE || (nt == FCLAW2D_PATCH_DOUBLESIZE))
        {
            pdata->on_coarsefine_interface = 1;
            if (nt == FCLAW2D_PATCH_HALFSIZE)
            {
                pdata->has_finegrid_neighbors = 1;
            }
        }
    }

    for (icorner = 0; icorner < 4; icorner++)
    {
        fclaw2d_patch_relation_t nt;
        int has_corner = pdata->corners[icorner];
        if (has_corner)
        {
            nt = pdata->corner_neighbors[icorner];
            if ((nt == FCLAW2D_PATCH_HALFSIZE) || (nt == FCLAW2D_PATCH_DOUBLESIZE))
            {
                pdata->on_coarsefine_interface = 1;
                if (nt == FCLAW2D_PATCH_HALFSIZE)
                {
                    pdata->has_finegrid_neighbors = 1;
                }
            }
        }
    }
    pdata->neighbors_set = 1;
}

void fclaw2d_patch_neighbors_reset(fclaw2d_patch_t* patch)
{
    fclaw2d_patch_data_t *pdata = fclaw2d_patch_get_user_data(patch);
    pdata->neighbors_set = 0;
}

int fclaw2d_patch_neighbor_type_set(fclaw2d_patch_t* patch)
{
    fclaw2d_patch_data_t *pdata = fclaw2d_patch_get_user_data(patch);
    return pdata->neighbors_set;
}


int fclaw2d_patch_has_finegrid_neighbors(fclaw2d_patch_t *patch)
{
    fclaw2d_patch_data_t *pdata = fclaw2d_patch_get_user_data(patch);
    return pdata->has_finegrid_neighbors;
}

int fclaw2d_patch_on_coarsefine_interface(fclaw2d_patch_t *patch)
{
    fclaw2d_patch_data_t *pdata = fclaw2d_patch_get_user_data(patch);
    return pdata->on_coarsefine_interface;
}


int
fclaw2d_patch_on_parallel_boundary (const fclaw2d_patch_t * patch)
{
    return patch->flags & FCLAW2D_PATCH_ON_PARALLEL_BOUNDARY ? 1 : 0;
}

int* fclaw2d_patch_block_corner_count(fclaw2d_domain_t* domain,
                                      fclaw2d_patch_t* this_patch)
{
    fclaw2d_patch_data_t *pdata = fclaw2d_patch_get_user_data(this_patch);
    return pdata->block_corner_count;
}

void fclaw2d_patch_set_block_corner_count(fclaw2d_domain_t* domain,
                                          fclaw2d_patch_t* this_patch,
                                          int icorner, int block_corner_count)
{
    fclaw2d_patch_data_t *pdata = fclaw2d_patch_get_user_data(this_patch);
    pdata->block_corner_count[icorner] = block_corner_count;
}


void
fclaw2d_domain_iterate_level_mthread (fclaw2d_domain_t * domain, int level,
                                      fclaw2d_patch_callback_t pcb, void *user)
{
#if (_OPENMP)
    int i, j;
    fclaw2d_block_t *block;
    fclaw2d_patch_t *patch;

    for (i = 0; i < domain->num_blocks; i++)
    {
        block = domain->blocks + i;
#pragma omp parallel for private(patch,j)
        for (j = 0; j < block->num_patches; j++)
        {
            patch = block->patches + j;
            if (patch->level == level)
            {
                pcb (domain, patch, i, j, user);
            }
        }
    }
#else
    fclaw_global_essentialf("fclaw2d_patch_iterator_mthread : We should not be here\n");
#endif
}


/* -------------------------------------------------------
   Relies on ClawPatch
   To get the links to ClawPatch out, we need to set up
   several virtual functions so that "patch" knows how to
   interpolate, average, etc.  Right now, only ClawPatch knows
   how to do this.
   -------------------------------------------------------- */

void fclaw2d_patch_data_new(fclaw2d_domain_t* domain,
                            fclaw2d_patch_t* this_patch)
{
    FCLAW_ASSERT(patch_vt()->patch_new != NULL);

    fclaw2d_domain_data_t *ddata = fclaw2d_domain_get_data(domain);

    /* Initialize user data */
    fclaw2d_patch_data_t *pdata = FCLAW2D_ALLOC(fclaw2d_patch_data_t, 1);
    this_patch->user = (void *) pdata;

    /* create new user data */
    FCLAW_ASSERT(patch_vt()->patch_new != NULL);
    pdata->user_patch = patch_vt()->patch_new();
    ++ddata->count_set_clawpatch;
    pdata->neighbors_set = 0;
}

void fclaw2d_patch_data_delete(fclaw2d_domain_t* domain,
                               fclaw2d_patch_t *this_patch)
{
    FCLAW_ASSERT(patch_vt()->patch_delete != NULL);
    fclaw2d_patch_data_t *pdata = (fclaw2d_patch_data_t*) this_patch->user;

    if (pdata != NULL)
    {
        fclaw2d_domain_data_t *ddata = fclaw2d_domain_get_data(domain);
        patch_vt()->patch_delete(pdata->user_patch);
        ++ddata->count_delete_clawpatch;

        FCLAW2D_FREE(pdata);
        this_patch->user = NULL;
    }
}

void fclaw2d_patch_initialize(fclaw2d_domain_t *domain,
                              fclaw2d_patch_t *this_patch,
                              int this_block_idx,
                              int this_patch_idx)
{
    FCLAW_ASSERT(patch_vt()->initialize != NULL);
    patch_vt()->initialize(domain,this_patch,this_block_idx,this_patch_idx);
}


void fclaw2d_patch_pack_local_ghost(fclaw2d_domain_t *domain,
                              fclaw2d_patch_t *this_patch,
                              double *patch_data,
                              int time_interp)
{
    FCLAW_ASSERT(patch_vt()->ghost_pack != NULL);
    patch_vt()->ghost_pack(domain,this_patch,patch_data,time_interp);
}

void fclaw2d_patch_build_remote_ghost(fclaw2d_domain_t *domain,
                                       fclaw2d_patch_t *this_patch,
                                       int blockno,
                                       int patchno,
                                       void *user)
{
    FCLAW_ASSERT(patch_vt()->build_ghost != NULL);

    patch_vt()->build_ghost(domain,this_patch,blockno,
                            patchno,(void*) user);
    if (patch_vt()->setup_ghost != NULL)
    {
        patch_vt()->setup_ghost(domain,this_patch,blockno,patchno);
    }
}

void fclaw2d_patch_unpack_remote_ghost(fclaw2d_domain_t* domain,
                                       fclaw2d_patch_t* this_patch,
                                       int this_block_idx,
                                       int this_patch_idx,
                                       double *qdata, fclaw_bool time_interp)
{
    FCLAW_ASSERT(patch_vt()->ghost_unpack != NULL);

    patch_vt()->ghost_unpack(domain, this_patch, this_block_idx,
                          this_patch_idx, qdata, time_interp);
}

size_t fclaw2d_patch_ghost_packsize(fclaw2d_domain_t* domain)
{
    FCLAW_ASSERT(patch_vt()->ghost_packsize != NULL);
    return patch_vt()->ghost_packsize(domain);
}

void fclaw2d_patch_alloc_local_ghost(fclaw2d_domain_t* domain,
                                     fclaw2d_patch_t* this_patch,
                                     void** q)
{
    FCLAW_ASSERT(patch_vt()->local_ghost_alloc != NULL);
    patch_vt()->local_ghost_alloc(domain, this_patch, q);
}

void fclaw2d_patch_free_local_ghost(fclaw2d_domain_t* domain,
                                    void **q)
{
    FCLAW_ASSERT(patch_vt()->local_ghost_free != NULL);
    patch_vt()->local_ghost_free(domain, q);
}

void cb_fclaw2d_patch_partition_pack(fclaw2d_domain_t *domain,
                                     fclaw2d_patch_t *this_patch,
                                     int this_block_idx,
                                     int this_patch_idx,
                                     void *user)
{
    FCLAW_ASSERT(patch_vt()->partition_pack != NULL);

    patch_vt()->partition_pack(domain,this_patch,this_block_idx,this_patch_idx,user);
}


double fclaw2d_patch_single_step_update(fclaw2d_domain_t *domain,
                                        fclaw2d_patch_t *this_patch,
                                        int this_block_idx,
                                        int this_patch_idx,
                                        double t,
                                        double dt)
{
    FCLAW_ASSERT(patch_vt()->single_step_update != NULL);

    double maxcfl = patch_vt()->single_step_update(domain,this_patch,this_block_idx,
                                                   this_patch_idx,t,dt);
    return maxcfl;
}


void cb_fclaw2d_patch_partition_unpack(fclaw2d_domain_t *domain,
                                     fclaw2d_patch_t *this_patch,
                                     int this_block_idx,
                                     int this_patch_idx,
                                     void *user)
{
    /* Create new data in 'user' pointer */
    fclaw2d_patch_data_new(domain,this_patch);

    fclaw2d_build_mode_t build_mode = FCLAW2D_BUILD_FOR_UPDATE;
    fclaw2d_patch_build(domain,this_patch,this_block_idx,
                        this_patch_idx,&build_mode);

    /* This copied q data from memory */
    FCLAW_ASSERT(patch_vt()->partition_pack != NULL);

    patch_vt()->partition_unpack(domain,
                                 this_patch,
                                 this_block_idx,
                                 this_patch_idx,
                                 user);
}

size_t fclaw2d_patch_partition_packsize(fclaw2d_domain_t* domain)
{
    FCLAW_ASSERT(patch_vt()->partition_packsize != NULL);
    return patch_vt()->partition_packsize(domain);
}

void fclaw2d_patch_build(fclaw2d_domain_t *domain,
                             fclaw2d_patch_t *this_patch,
                             int blockno,
                             int patchno,
                             void *user)
{
    FCLAW_ASSERT(patch_vt()->build != NULL);

    patch_vt()->build(domain,this_patch,blockno,patchno,user);

    if (patch_vt()->setup != NULL)
    {
        /* The setup routine should check to see if this is a ghost patch and
           optimize accordingly.  For example, interior data is not generally
           needed (beyond what is needed for averaging) */
        patch_vt()->setup(domain,this_patch,blockno,patchno);
    }
}

void fclaw2d_patch_build_from_fine(fclaw2d_domain_t *domain,
                                       fclaw2d_patch_t *fine_patches,
                                       fclaw2d_patch_t *coarse_patch,
                                       int blockno,
                                       int coarse_patchno,
                                       int fine0_patchno,
                                       fclaw2d_build_mode_t build_mode)
{
    FCLAW_ASSERT(patch_vt()->build_from_fine != NULL);

    patch_vt()->build_from_fine(domain,
                                fine_patches,
                                coarse_patch,
                                blockno,
                                coarse_patchno,
                                fine0_patchno,
                                build_mode);
    if (patch_vt()->setup != NULL && build_mode == FCLAW2D_BUILD_FOR_UPDATE)
    {
        /* We might want to distinguish between new fine grid patches, and
           new coarse grid patches.  In the latter case, we might just average
           aux array info, for example, rather than recreate it from scratch.
           Something like a general "build from fine" routine might be needed */
        patch_vt()->setup(domain,coarse_patch,blockno,coarse_patchno);
    }
}

void fclaw2d_patch_restore_step(fclaw2d_domain_t* domain,
                                    fclaw2d_patch_t* this_patch)
{
    FCLAW_ASSERT(patch_vt()->restore_step != NULL);

    patch_vt()->restore_step(domain, this_patch);
}

/* This is called from libraries routines (clawpack4.6, clawpack5, etc) */
void fclaw2d_patch_save_step(fclaw2d_domain_t* domain,
                             fclaw2d_patch_t* this_patch)
{
    FCLAW_ASSERT(patch_vt()->save_step != NULL);
    patch_vt()->save_step(domain, this_patch);
}

void fclaw2d_patch_interpolate_face(fclaw2d_domain_t *domain,
                                    fclaw2d_patch_t *coarse_patch,
                                    fclaw2d_patch_t *fine_patch,
                                    int idir,
                                    int iside,
                                    int p4est_refineFactor,
                                    int refratio,
                                    fclaw_bool time_interp,
                                    int igrid,
                                    fclaw2d_transform_data_t* transform_data)
{
    FCLAW_ASSERT(patch_vt()->interpolate_face != NULL);

    patch_vt()->interpolate_face(domain,coarse_patch,fine_patch,idir,
                                 iside,p4est_refineFactor,refratio,
                                 time_interp,igrid,transform_data);
}

void fclaw2d_patch_average_face(fclaw2d_domain_t *domain,
                                fclaw2d_patch_t *coarse_patch,
                                fclaw2d_patch_t *fine_patch,
                                int idir,
                                int iface_coarse,
                                int p4est_refineFactor,
                                int refratio,
                                fclaw_bool time_interp,
                                int igrid,
                                fclaw2d_transform_data_t* transform_data)
{
    FCLAW_ASSERT(patch_vt()->average_face != NULL);

    patch_vt()->average_face(domain,coarse_patch,fine_patch,idir,
                          iface_coarse,p4est_refineFactor,
                          refratio,time_interp,igrid,
                          transform_data);
}

void fclaw2d_patch_copy_face(fclaw2d_domain_t *domain,
                             fclaw2d_patch_t *this_patch,
                             fclaw2d_patch_t *neighbor_patch,
                             int iface,
                             int time_interp,
                             fclaw2d_transform_data_t *transform_data)
{
    FCLAW_ASSERT(patch_vt()->copy_face != NULL);

    patch_vt()->copy_face(domain,this_patch,neighbor_patch,iface,
                       time_interp,transform_data);

}

void fclaw2d_patch_copy_corner(fclaw2d_domain_t *domain,
                               fclaw2d_patch_t *this_patch,
                               fclaw2d_patch_t *corner_patch,
                               int icorner,
                               int time_interp,
                               fclaw2d_transform_data_t *transform_data)
{
    FCLAW_ASSERT(patch_vt()->copy_corner != NULL);

    patch_vt()->copy_corner(domain,this_patch,corner_patch,
                         icorner,time_interp,transform_data);
}

void fclaw2d_patch_average_corner(fclaw2d_domain_t *domain,
                                  fclaw2d_patch_t *coarse_patch,
                                  fclaw2d_patch_t *fine_patch,
                                  int coarse_corner,
                                  int refratio,
                                  fclaw_bool time_interp,
                                  fclaw2d_transform_data_t* transform_data)
{
    FCLAW_ASSERT(patch_vt()->average_corner != NULL);
    patch_vt()->average_corner(domain,coarse_patch,fine_patch,coarse_corner,
                            refratio,time_interp,transform_data);
}


void fclaw2d_patch_interpolate_corner(fclaw2d_domain_t* domain,
                                      fclaw2d_patch_t* coarse_patch,
                                      fclaw2d_patch_t* fine_patch,
                                      int coarse_corner,
                                      int refratio,
                                      fclaw_bool time_interp,
                                      fclaw2d_transform_data_t* transform_data)
{
    FCLAW_ASSERT(patch_vt()->interpolate_corner != NULL);
    patch_vt()->interpolate_corner(domain,coarse_patch,fine_patch,
                                coarse_corner,refratio,time_interp,
                                transform_data);
}

int fclaw2d_patch_tag4refinement(fclaw2d_domain_t *domain,
                                      fclaw2d_patch_t *this_patch,
                                      int blockno, int patchno,
                                      int initflag)
{
    FCLAW_ASSERT(patch_vt()->tag4refinement != NULL);

    return patch_vt()->tag4refinement(domain,this_patch,blockno,
                                   patchno, initflag);
}

int fclaw2d_patch_tag4coarsening(fclaw2d_domain_t *domain,
                                      fclaw2d_patch_t *fine_patches,
                                      int blockno,
                                      int patchno)
{
    FCLAW_ASSERT(patch_vt()->tag4coarsening != NULL);

    return patch_vt()->tag4coarsening(domain,fine_patches,
                                   blockno, patchno);
}

void fclaw2d_patch_interpolate2fine(fclaw2d_domain_t* domain,
                                             fclaw2d_patch_t* coarse_patch,
                                             fclaw2d_patch_t* fine_patches,
                                             int this_blockno, int coarse_patchno,
                                             int fine0_patchno)

{
    FCLAW_ASSERT(patch_vt()->interpolate2fine != NULL);

    patch_vt()->interpolate2fine(domain,coarse_patch,fine_patches,
                                 this_blockno,coarse_patchno,
                                 fine0_patchno);
}

void fclaw2d_patch_average2coarse(fclaw2d_domain_t *domain,
                                         fclaw2d_patch_t *fine_patches,
                                         fclaw2d_patch_t *coarse_patch,
                                         int blockno, int fine0_patchno,
                                         int coarse_patchno)

{
    FCLAW_ASSERT(patch_vt()->average2coarse != NULL);

    patch_vt()->average2coarse(domain,fine_patches,coarse_patch,
                               blockno,fine0_patchno,coarse_patchno);
}


void fclaw2d_patch_setup_timeinterp(fclaw2d_domain_t* domain,
                                    fclaw2d_patch_t *this_patch,
                                    double alpha)
{
    FCLAW_ASSERT(patch_vt()->setup_timeinterp != NULL);

    patch_vt()->setup_timeinterp(domain,this_patch,alpha);
}


void fclaw2d_patch_write_file(fclaw2d_domain_t *domain,
                              fclaw2d_patch_t *this_patch,
                              int this_block_idx,
                              int this_patch_idx,
                              int iframe,int patch_num,
                              int level)
{
    FCLAW_ASSERT(patch_vt()->write_file != NULL);

    patch_vt()->write_file(domain,this_patch,this_block_idx,
                           this_patch_idx,iframe,patch_num,level);
}

void fclaw2d_patch_write_header(fclaw2d_domain_t* domain,
                                int iframe)
{
    FCLAW_ASSERT(patch_vt()->write_header != NULL);
    patch_vt()->write_header(domain,iframe);
}

void fclaw2d_patch_physical_bc(fclaw2d_domain_t *domain,
                               fclaw2d_patch_t *this_patch,
                               int this_block_idx,
                               int this_patch_idx,
                               double t,
                               double dt,
                               fclaw_bool *intersects_bc,
                               fclaw_bool time_interp)
{
    FCLAW_ASSERT(patch_vt()->physical_bc != NULL);
    patch_vt()->physical_bc(domain,this_patch,this_block_idx,this_patch_idx,
                            t,dt,intersects_bc,time_interp);
}
