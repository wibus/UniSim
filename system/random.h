#ifndef RANDOM_H
#define RANDOM_H

#include <GLM/glm.hpp>


namespace unisim
{

double *halton ( int i, int m );
double *halton_base ( int i, int m, int b[] );
int halton_inverse ( double r[], int m );
double *halton_sequence ( int i1, int i2, int m );
int i4vec_sum ( int n, int a[] );
int prime ( int n );
double r8_mod ( double x, double y );

}

#endif // RANDOM_H
