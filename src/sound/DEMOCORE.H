/* Scheduling values shared by the original demo and host logic tests. */
static unsigned melody[4]={72,76,79,76};
static unsigned envelope[16]={8,7,7,8,8,9,9,10,10,11,11,12,12,13,14,15};
static unsigned percussion(unsigned age)
{
    unsigned a=8+age;
    return a>14?15:a;
}
static unsigned articulation(unsigned age)
{
    return envelope[age<16?age:15];
}
static unsigned leadPeriod(unsigned age)
{
    if(age<8)return 427;
    return (age&1)?424:430;
}
