from PIL import Image, ImageDraw, ImageFont
from pathlib import Path
out = Path(__file__).parent
# Original, code-native 320x200 diagram: hard-edged, no antialiasing, 16 colours.
# Colour indices follow the Tandy IRGB order (bit0 blue, bit1 green, bit2 red,
# bit3 intensity), so the swatches are the very colours the hardware can show.
IRGB = [(0,0,0),(0,0,170),(0,170,0),(0,170,170),(170,0,0),(170,0,170),
        (170,85,0),(170,170,170),(85,85,85),(85,85,255),(85,255,85),
        (85,255,255),(255,85,85),(255,85,255),(255,255,85),(255,255,255)]
W, H = 320, 200
im = Image.new('P', (W, H), 0)
pal = []
for c in IRGB:
    pal += list(c)
im.putpalette(pal + [0, 0, 0] * 240)
d = ImageDraw.Draw(im)
f = ImageFont.load_default()
WHITE, GRAY, BLUE, YELLOW, RED, GREEN = 15, 7, 9, 14, 12, 10
def label(x, y, s, c=WHITE): d.text((x, y), s, fill=c, font=f)
def center(y, s, c=WHITE):
    b = d.textbbox((0, 0), s, font=f)
    label((W - (b[2] - b[0])) // 2, y, s, c)
def arrow(a, b, c=WHITE):
    d.line([a, b], fill=c)
    ax, ay = b
    d.polygon([(ax, ay), (ax - 5, ay - 3), (ax - 5, ay + 3)], fill=c)

center(4, 'WHY PALETTE CYCLING IS CHEAP', YELLOW)

# Top row: one byte of video memory = two 4-bit pixels, first in the high nibble.
label(6, 18, 'VIDEO MEMORY BYTE', GRAY)
d.rectangle([6, 30, 70, 48], outline=WHITE)
d.line([(38, 30), (38, 48)], fill=WHITE)
label(19, 35, '3', WHITE)
label(51, 35, '5', WHITE)
label(80, 28, 'TWO PIXELS PER BYTE;', GRAY)
label(80, 40, 'HIGH NIBBLE IS FIRST', GRAY)

# Middle row: sixteen palette registers, each holding one 4-bit IRGB colour.
label(6, 60, 'PALETTE REGISTERS', GRAY)
PX0, PW = 6, 19
def reg_x(n): return PX0 + n * PW
for n in range(16):
    x = reg_x(n)
    label(x + 5, 72, '%X' % n, WHITE)
    fill = IRGB[n] if n in (3, 5) else IRGB[(n * 5 + 1) % 16]
    d.rectangle([x, 84, x + PW - 3, 98], fill=fill, outline=GRAY)
arrow((22, 49), (reg_x(3) + 7, 83), YELLOW)
arrow((54, 49), (reg_x(5) + 7, 83), YELLOW)

# Bottom row: the screen shows whatever the selected register holds.
label(6, 108, 'ON THE SCREEN', GRAY)
d.rectangle([6, 120, 70, 130], fill=IRGB[3], outline=WHITE)
d.rectangle([71, 120, 135, 130], fill=IRGB[5], outline=WHITE)
arrow((reg_x(3) + 7, 99), (38, 119), YELLOW)
arrow((reg_x(5) + 7, 99), (103, 119), YELLOW)
label(150, 112, 'WRITE-ONLY: CANNOT BE', GRAY)
label(150, 124, 'READ BACK, ONLY LOADED', GRAY)

# The write sequence.
d.line([(6, 140), (314, 140)], fill=GRAY)
label(6, 146, 'OUT 3DA, 10h+N    (SELECT REGISTER N)', GREEN)
label(6, 158, 'OUT 3DE, IRGB     (NEW COLOUR, 4 BITS)', GREEN)
label(6, 170, 'OUT 3DA, 01h      (VIDEO BACK ON)', GREEN)
center(188, 'CHANGE ONE REGISTER: EVERY PIXEL USING IT CHANGES', RED)

im.save(out / 'SCREEN_320.PNG', bits=4, optimize=False)
print('Saved', out / 'SCREEN_320.PNG')
