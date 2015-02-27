c     =====================================================
       subroutine qinit(maxmx, maxmy, meqn,mbc,mx,my,
     &      xlower,ylower,dx,dy,q,maux,aux)
c     =====================================================

c     # Set initial conditions for q.
c     # Sample scalar equation with data that is piecewise constant with
c     # q = 1.0  if  0.1 < x < 0.6   and   0.1 < y < 0.6
c     #     0.1  otherwise

       implicit none

       integer maxmx, maxmy, meqn, mbc, mx, my, maux
       double precision xlower, ylower, dx, dy
       double precision q(1-mbc:mx+mbc, 1-mbc:my+mbc, meqn)
       double precision aux(1-mbc:mx+mbc, 1-mbc:my+mbc, maux)
       integer blockno, fc2d_clawpack46_get_block;

       integer i, j, mq
       double precision xlow,ylow,w

       blockno = fc2d_clawpack46_get_block();

       do mq = 1,meqn
          do i = 1-mbc,mx+mbc
             xlow = xlower + (i-0.5d0)*dx
             do j = 1-mbc,my+mbc
                ylow = ylower + (j-1d0)*dy
                call cellave2(blockno,xlow,ylow,dx,dy,w)
                q(i,j,mq) = w
             enddo
          enddo
       enddo

       return
       end

      double precision function  fdisc(blockno,xc,yc)
      implicit none

      double precision xc,yc, xp, yp, zp
      integer blockno
      integer*8 cont, get_context

      cont = get_context()

      call fclaw2d_map_c2m(cont,
     &      blockno,xc,yc,xp,yp,zp)

      fdisc = xp-0.5d0
      end
