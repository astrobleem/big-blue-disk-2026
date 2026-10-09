from PIL import Image, ImageDraw, ImageFont
from pathlib import Path
import math
out=Path(__file__).parent
# Original, code-native drawing: hard-edged 1-bit geometry, no antialiasing.
im=Image.new('1',(320,200),0)
d=ImageDraw.Draw(im)
f=ImageFont.load_default()
def label(x,y,s): d.text((x,y),s,fill=1,font=f)
def center(y,s):
 b=d.textbbox((0,0),s,font=f)
 label((320-(b[2]-b[0]))//2,y,s)
def ray(a,b):
 d.line([a,b],fill=1,width=1)
 # Small direction chevron before the aperture.
 t=.27
 x=a[0]+(b[0]-a[0])*t; y=a[1]+(b[1]-a[1])*t
 angle=math.atan2(b[1]-a[1],b[0]-a[0])
 for sign in [-1,1]:
  end=(round(x-5*math.cos(angle)+sign*3*math.sin(angle)),round(y-5*math.sin(angle)-sign*3*math.cos(angle)))
  d.line([(round(x),round(y)),end],fill=1)
center(6,'WHY THE IMAGE IS UPSIDE DOWN')
label(18,34,'OBJECT')
label(181,34,'DARK BOX')
label(255,34,'SCREEN')
# Opaque wall and dark chamber. A visible gap marks the small aperture.
d.line([(155,57),(274,57),(274,143),(155,143)],fill=1)
d.line([(155,57),(155,96)],fill=1,width=3)
d.line([(155,104),(155,143)],fill=1,width=3)
d.line([(274,57),(274,143)],fill=1,width=3)
# Object and inverted image have matching endpoints relative to the hole.
d.line([(36,63),(36,137)],fill=1,width=3)
d.polygon([(36,63),(28,76),(44,76)],fill=1)
d.line([(274,63),(274,137)],fill=1,width=3)
d.polygon([(274,137),(266,124),(282,124)],fill=1)
ray((36,63),(274,137))
ray((36,137),(274,63))
# Midpoint is exactly (155,100); the two rays continue straight.
label(16,149,'UPRIGHT')
label(248,149,'INVERTED')
label(88,95,'PINHOLE')
d.line([(135,100),(150,100)],fill=1)
label(128,44,'WALL')
center(184,'TOP -> BOTTOM   BOTTOM -> TOP')
# Two used colors in a palette suitable for a 16-color target.
pal=im.convert('L').point(lambda value: 1 if value else 0).convert('P')
pal.putpalette([0,0,0,255,255,255]+[0,0,0]*254)
pal.save(out/'CAMERA_320.PNG',bits=4,optimize=False)
# Exact horizontally doubled optional 640x200 monochrome asset.
im.resize((640,200),Image.Resampling.NEAREST).save(out/'CAMERA_640_MONO.PNG')
print('Saved',out/'CAMERA_320.PNG')
