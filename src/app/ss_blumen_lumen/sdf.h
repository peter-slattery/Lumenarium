/* date = March 29th 2021 10:41 pm */

#ifndef SDF_H
#define SDF_H

internal r32
SDF_Sphere(v3 Point, v3 Center, r32 Radius)
{
    r32 Result = V3Mag(Point - Center) - Radius;
    return Result;
}

internal r32
SDF_SphereNormalized(v3 Point, v3 Center, r32 Radius)
{
    r32 Result = SDF_Sphere(Point, Center, Radius) / Radius;
    return Result;
}

#endif //SDF_H
