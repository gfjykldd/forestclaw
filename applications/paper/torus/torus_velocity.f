c     # ------------------------------------------------------------
c     # Prescribes velocity fields for the unit sphere.
c     # 
c     # ------------------------------------------------------------
   
      subroutine torus_velocity_derivs(x,y,t, u,vcart,derivs,flag)
      implicit none

      double precision x, y, u(2), vcart(3), t
      double precision derivs(4)
      integer flag

      double precision pi, pi2
      common /compi/ pi, pi2

      double precision revs_per_s
      common /stream_comm/ revs_per_s

      integer example
      common /example_comm/ example  

      double precision s, pim

      flag = 0
      if (example .eq. 0) then
          u(1) = revs_per_s
          u(2) = 0
          derivs(1) = 0
          derivs(2) = 0
          derivs(3) = 0
          derivs(4) = 0
      elseif (example .eq. 1) then
          s = sqrt(2.d0)
          pim = 8*pi
          u(1) = s*cos(pim*x)
          u(2) = s*sin(pim*y)   
c         # uderivs = [u1x u1y; u2x u2y]          
          derivs(1) = -s*pim*sin(pim*x)
          derivs(2) = 0;
          derivs(3) = 0; 
          derivs(4) = s*pim*cos(pim*y)
      endif


      end



c     # ----------------------------------------------------------------
c     #                       Public interface
c     # ----------------------------------------------------------------


c     # ---------------------------------------------
c     # Called from setaux
c     # 
c     #    -- used to compute velocity at faces
c     # ---------------------------------------------
      subroutine velocity_components_cart(x,y,t,vcart)
      implicit none

      double precision x,y,t, u(2), vcart(3), uderivs(4)
      double precision t1(3), t2(3)
      integer flag, k


      call torus_velocity_derivs(x,y,t, u,vcart,uderivs,flag)

      if (flag .eq. 0) then
c         # Velocity components are given in spherical components
c         # and must be converted to Cartesian
          call map_covariant_basis(x, y, t1,t2)

          do k = 1,3
              vcart(k) = u(1)*t1(k) + u(2)*t2(k)
          enddo
      endif

      end


c     # ------------------------------------------------------------
c     # Called from map_divergence
c     # 
c     #    -- Needed to define ODE system to get exact solution
c     # ------------------------------------------------------------
      subroutine velocity_derivs(x,y,t, u, vcart, derivs, flag)
      implicit none

      double precision x,y,t, u(2), vcart(3), derivs(4)
      double precision t1(3), t2(3), t1n2, t2n2, map_dot
      double precision t1inv(3), t2inv(3)
      integer flag

      call torus_velocity_derivs(x,y,t, u,vcart,derivs,flag)

      if (flag .eq. 1) then
c         # Velocity components are given in Cartesian components
          call map_covariant_basis(x, y, t1,t2)
          t1n2 = map_dot(t1,t1)
          t2n2 = map_dot(t2,t2)
          u(1) = map_dot(vcart,t1)/t1n2
          u(2) = map_dot(vcart,t2)/t2n2

c          call map_contravariant_basis(x, y, t1inv,t2inv)
c          # Convert Cartesian derivatives to u(1),u(2) derivatives
c          # 
c          #   grad v1 = (dv1/dx)*t1c + (dv1/dy)*t2c + (dv1/dz)*t3c
c          #   grad v2 = (dv2/dx)*t1c + (dv2/dy)*t2c + (dv2/dz)*t3c
c          # .....
c          # Don't think this is needed ...

      endif

      end

c     # ------------------------------------------------------------
c     # Called from qexact
c     # 
c     #  -- components relative to basis are needed.
c     # ------------------------------------------------------------
      subroutine velocity_components(x,y,t,u)
      implicit none

      double precision x,y,t, u(2), vcart(3), derivs(4)
      integer flag

      call velocity_derivs(x,y,t, u,vcart,derivs,flag)

      end



