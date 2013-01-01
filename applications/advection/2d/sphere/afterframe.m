s = 1e-2;
% axis([-1-s 1+s -1-s 1+s -1-s 1+s])
axis image;
daspect([1 1 1]);
axis off;
yrbcolormap;
% colormap(jet);

cv = 0.1:0.1:0.9;
% drawcontourlines(cv);
caxis([0 1]);

showgridlines(1:3)
setpatchborderprops(1:6,'linewidth',2);
hidepatchborders(6);


setviews;
view(3);
% view([47.5 6]);
% view(vtop);

NoQuery = 0;
MaxFrames = 192;
prt = false;
if (prt)
  filename = 'swirl000.png';
  str = num2str(Frame);
  len = length(str);
  filename(8-len+1:8) = str;
  pstr = ['print -dpng ',filename];
  disp(pstr);
  eval(pstr);
end;

clear afterframe;
clear mapc2m;
clear mapc2m_new;
