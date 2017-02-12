double heat (double x, double y, double t)
{
    return 0;
}

double init (double x, double y)
{
    return 0;
}

double border (double x, double y, double t, double X, double Y)
{
    if ((y>Y/5) && (y<Y*4/5)){
        return 0.5;
    }
    if ((x>X/5) && (x<X*4/5)){
        return 0.5;
    }
    return -1;
}
