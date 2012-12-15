#include <float.h>
#include <string.h>
#include <stdio.h>

#include "vec3.h"

body *particle = NULL;
double *ctimes = NULL;
int collider1, collider2;
int n = 5;
int N;
double ETA = 0.68;
double SIGMA;

double runtime;
double min_time;
double pressure;
double dp;
double kinetic;
double temperature;
int hits;

double get_kinetic()
{
    double tmp = 0;
    int k;
    for(k = 0; k < N; k++)
        tmp += vec3_dot(particle[k].v,particle[k].v);
    return tmp/2;
}

double get_min()
{
    int i,j;
    double minimum = ctimes[1];
    collider1 = 0;
    collider2 = 1;
    for(i = 0; i < N; i++)
        for(j = i+1; j < N; j++)
            if(ctimes[i*N+j] <= minimum){
                minimum = ctimes[i*N+j];
                collider1 = i;
                collider2 = j;
            }
    return minimum;
}

double get_collision_time(int i, int j)
{
    vec3 dr, dv;
    double t, dx, dy, dz, rr, rv, vv, delta, collision_time = DBL_MAX;

    for(dx = -1; dx <= 1 ; dx++){
        for(dy = -1; dy <= 1; dy++){
            for(dz = -1; dz <= 1; dz++){
                dr.x = dx + particle[j].r.x - particle[i].r.x;
                dr.y = dy + particle[j].r.y - particle[i].r.y;
                dr.z = dz + particle[j].r.z - particle[i].r.z;

                dv.x = particle[j].v.x - particle[i].v.x;
                dv.y = particle[j].v.y - particle[i].v.y;
                dv.z = particle[j].v.z - particle[i].v.z;

                rr = vec3_dot(dr, dr);
                rv = vec3_dot(dr, dv);
                vv = vec3_dot(dv, dv);
                delta = rv*rv-vv*(rr-SIGMA*SIGMA);

                if((rv < 0)&&(delta > 0)){
                    t = (-rv-sqrt(delta))/vv;
                    if(t < collision_time)
                        collision_time = t;
                }
            }
        }
    }
    return collision_time;
}

void collide()
{
    int i, j;
    for(i = 0; i < N; i++)
        for(j = i+1; j < N; j++)
            if(i == collider1 || j == collider1 || i == collider2 || j == collider2 || collider1 == collider2)
                ctimes[i*N+j] = get_collision_time(i,j);
            else
                ctimes[i*N+j] -= min_time;
}

void clear()
{
    if(particle != NULL)
        free(particle);
    if(ctimes != NULL)
        free(ctimes);
}

void reset()
{
    runtime = 0;
    dp = 0;
    pressure = 0;
    hits = 0;
}

void init()
{
    N = 2*n*n*n;
    SIGMA = cbrt(ETA*1.909859317/N);

    clear();
    particle = (body*)malloc(N*sizeof(body));
    ctimes = (double*)malloc(N*N*sizeof(body));

    int i,j,k,l;
    for(l = i = 0; i < n; i++)
        for(j = 0; j < n; j++)
            for(k = 0; k < n; k++){
                particle[l].r.x = i*1.0/n;
                particle[l].r.y = j*1.0/n;
                particle[l].r.z = k*1.0/n;
                particle[N/2+l].r.x = 1.0/n/2.0 + i*1.0/n;
                particle[N/2+l].r.y = 1.0/n/2.0 + j*1.0/n;
                particle[N/2+l].r.z = 1.0/n/2.0 + k*1.0/n;
                l++;
            }

    vec3 tmp = vec3_new(0.0, 0.0, 0.0);
    for(k = 0; k < N; k++){
        particle[k].c = vec3_new(_rand(), _rand(), _rand());
        particle[k].v = vec3_new(_rand()*2-1, _rand()*2-1, _rand()*2-1);
        tmp.x += particle[k].v.x;
        tmp.y += particle[k].v.y;
        tmp.z += particle[k].v.z;
    }

    for(k = 0; k < N; k++){
        particle[k].v.x -= tmp.x/N;
        particle[k].v.y -= tmp.y/N;
        particle[k].v.z -= tmp.z/N;
    }
    double norm = 0;
    for(k = 0; k < N; k++)
        norm += vec3_dot(particle[k].v, particle[k].v);
    norm = 2 * norm / (3 * N);
    norm = sqrt(norm);
    for(k = 0; k < N; k++){
        particle[k].v.x /= norm;
        particle[k].v.y /= norm;
        particle[k].v.z /= norm;
    }

    collider1 = 0;
    collider2 = 0;

    kinetic = get_kinetic();
    temperature = 2*kinetic/(3*N);

    collide();
    reset();
}

void run()
{
    min_time = get_min();

    int k;
    for(k = 0; k < N; k++){
        particle[k].r.x += particle[k].v.x * min_time;
        particle[k].r.y += particle[k].v.y * min_time;
        particle[k].r.z += particle[k].v.z * min_time;

        particle[k].r.x = fmod(particle[k].r.x,1);
        particle[k].r.y = fmod(particle[k].r.y,1);
        particle[k].r.z = fmod(particle[k].r.z,1);

        if(particle[k].r.x < 0) particle[k].r.x += 1;
        if(particle[k].r.y < 0) particle[k].r.y += 1;
        if(particle[k].r.z < 0) particle[k].r.z += 1;
    }

    vec3 dr, dv, tmp;

    dr.x = particle[collider2].r.x - particle[collider1].r.x;
    dr.y = particle[collider2].r.y - particle[collider1].r.y;
    dr.z = particle[collider2].r.z - particle[collider1].r.z;

    double dx, dy, dz;
    for(dx = -1; dx <= 1 ; dx++){
        for(dy = -1; dy <= 1; dy++){
            for(dz = -1; dz <= 1; dz++){
                tmp.x = dx + particle[collider2].r.x - particle[collider1].r.x;
                tmp.y = dy + particle[collider2].r.y - particle[collider1].r.y;
                tmp.z = dz + particle[collider2].r.z - particle[collider1].r.z;

                if(vec3_dot(tmp,tmp) < vec3_dot(dr,dr)){
                    dr = tmp;
                }
            }
        }
    }

    dv.x = particle[collider1].v.x - particle[collider2].v.x;
    dv.y = particle[collider1].v.y - particle[collider2].v.y;
    dv.z = particle[collider1].v.z - particle[collider2].v.z;

    tmp.x = vec3_dot(dv,dr)*dr.x/vec3_dot(dr,dr);
    tmp.y = vec3_dot(dv,dr)*dr.y/vec3_dot(dr,dr);
    tmp.z = vec3_dot(dv,dr)*dr.z/vec3_dot(dr,dr);

    particle[collider1].v.x -= tmp.x;
    particle[collider1].v.y -= tmp.y;
    particle[collider1].v.z -= tmp.z;

    particle[collider2].v.x += tmp.x;
    particle[collider2].v.y += tmp.y;
    particle[collider2].v.z += tmp.z;

    collide();

    hits++;
    runtime += min_time;
    dp += vec3_mod(dv);
    pressure = 1+SIGMA*dp/(2*kinetic*runtime);
}

void print()
{
    FILE *v = fopen("speed.dat","w");
    FILE *r = fopen("position.dat","w");
    FILE *f = fopen("data.dat","a");

    int k;
    for(k = 0; k < N; k++){
        fprintf(v, "%e\t%e\t%e\n", particle[k].v.x, particle[k].v.y, particle[k].v.z);
        fprintf(r, "%e\t%e\t%e\n", particle[k].r.x, particle[k].r.y, particle[k].r.z);
    }

    fprintf(f, "%d\t%lf\t%lf\t%lf\n", N, ETA, pressure, pressure/N/temperature-1);

    fclose(v);
    fclose(r);
    fclose(f);
}

