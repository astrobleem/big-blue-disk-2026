/* Fine clock, from Radio Shack Rave (same authors, GPLv3). BIOS tick count
 * plus the phase of PIT counter 0, which is only ever latched and read.
 * PIT0 is never reprogrammed. The BIOS runs it in mode 3, where the latched
 * count runs 65536..0 twice per tick, so a single read is ambiguous by half a
 * tick. Readings are kept monotonic: within one tick the first-half answer is
 * used until the count wraps under the previous reading, then the second
 * half. That holds while reads are less than half a tick (27 ms) apart,
 * which every frame loop here guarantees. A pending IRQ0 at read time means
 * the new period has begun but the BIOS has not counted it yet. If an odd
 * count is ever seen, the BIOS is using mode 2 and the phase is direct.
 * Units: 1/256 BIOS tick (about 0.21 ms).
 *
 * Capture modes replace this with a virtual clock that advances exactly
 * half a tick per presented frame. The episode calls the clock from inside
 * long drawing loops, so reads stay well under half a tick apart.
 */
#ifndef HOST
static unsigned irq_save(void)
{
    unsigned flags;
    _asm pushf
    _asm pop flags
    _asm cli
    return flags;
}
static void irq_restore(unsigned flags)
{
    _asm push flags
    _asm popf
}
#endif
#define BIOS_DAY 0x1800b0UL
#define RENDER_STEP 128
static unsigned long clk_last,clk_wrap,clk_vtime;
static int clk_mode2,clk_virtual;
static unsigned long clk_real(void)
{
#ifndef HOST
    unsigned flags,lo,hi,c,irr;unsigned long t,a;
#endif
#ifdef HOST
    return 0;
#else
    flags=irq_save();
    outp(0x43,0);lo=inp(0x40);hi=inp(0x40);
    t=*(unsigned long far *)0x0040006cUL;
    outp(0x20,0x0a);irr=inp(0x20);
    irq_restore(flags);
    c=lo|(hi<<8);
    if(c&1)clk_mode2=1;
    t+=clk_wrap;
    if(irr&1)t++;
    /* Midnight: the BIOS count restarts at zero. Keep our timeline running. */
    if((t<<8)+0x10000UL<clk_last && (clk_last>>8)-t>0x100000UL) {
        clk_wrap+=BIOS_DAY;t+=BIOS_DAY;
    }
    if(clk_mode2)a=(t<<8)+(((unsigned)(0-c))>>8);
    else {
        a=(t<<8)+(((unsigned)(0-c))>>9);
        if(a<clk_last)a+=128;
    }
    if(a<clk_last)a=clk_last;
    clk_last=a;
    return a;
#endif
}
static unsigned long clk_fine(void)
{
    return clk_virtual?clk_vtime:clk_real();
}
/* Wait for the start of vertical retrace (status bit 3), bounded. */
static int vsync_on=1;
static void vsync_wait(void)
{
#ifndef HOST
    unsigned n;
    if(clk_virtual || !vsync_on)return;
    for(n=0;n<30000 && (inp(0x3da)&8);n++);
    for(n=0;n<30000 && !(inp(0x3da)&8);n++);
#endif
}
