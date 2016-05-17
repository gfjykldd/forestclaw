      subroutine torus_compute_error(mx,my,mbc,meqn,
     &      dx,dy,xlower,ylower,t, q,error)
      implicit none

      integer mx,my,mbc,meqn
      double precision dx, dy, xlower, ylower, t
      double precision q(1-mbc:mx+mbc,1-mbc:my+mbc,meqn)
      double precision error(1-mbc:mx+mbc,1-mbc:my+mbc,meqn)

      integer i,j,m
      double precision xc,yc, qexact

c     # Assume a single field variable only
      do j = 1,my
         do i = 1,mx
            xc = xlower + (i-0.5)*dx
            yc = ylower + (j-0.5)*dy
            error(i,j,m) = abs(q(i,j,m) - qexact(xc,yc,t));
         enddo
      enddo


      end


      double precision function qexact(xc,yc,t)
      implicit none

      double precision xc,yc,t
      double precision x0, y0, u0, v0

      u0 = 1
      v0 = 1

      x0 = 0.5d0
      y0 = 0.5d0


      qexact = 0.d0


      end
