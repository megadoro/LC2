#include"potts.h"

int main(){

    set_corr(1); /* enables the computation of correlations */

    double b = 0.8;
    for(b = 0.8; b < 0.96; b += 0.01 )
        dump_data(64, b, SW, 10000);
    return 0;
}
