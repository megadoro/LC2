#ifndef HARDCORE2D
#define HARDCORE2D

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <float.h>
#include "mersenne.h"
#include "vec2.h"

double SIGMA = 0.0f;            /* diameter of the disks */
int n_particles = 100;          /* number of particles */
int n_history = 10000;          /* number of time steps of the simulation */
int collider[2];                /* indices of the two colliding particles */
double time_step = 0.03f;

int n_collisions;
double runtime;
double pressure;
double dp;

int idx_history_time;
double time_prec;
double min_time;


typedef struct body{
    double pos[DIMENSION];      /* position */
    double mom[DIMENSION];      /* momentum */
    double distance;            /* distance traveled */
    double last_collision_time; /* time of the last collision of the particle */
    int n_collisions;  /* collisions made */
}body;

body *particle = NULL;      /* particle list */
body *history = NULL;       /* simulation history */
double *collision_table = NULL;   /* collision table */


void print_pos()
{
    FILE *f = fopen("data/position.dat","w");
    int i,j;
    for(i = 0; i < n_particles; i++){
        for(j = 0; j < DIMENSION; j++)
            fprintf(f, "%e\t", particle[i].pos[j]);
        fprintf(f, "\n");
    }
    fclose(f);
}

void print_mom()
{
    FILE *f = fopen("data/momentum.dat","w");
    int i,j;
    for(i = 0; i < n_particles; i++){
        for(j = 0; j < DIMENSION; j++)
            fprintf(f, "%e\t", particle[i].mom[j]);
        fprintf(f, "\n");
    }
    fclose(f);
}

void print_distribution()
{
    int i;
    FILE *f = fopen("data/boltzmann.dat","w");
    for(i = 0; i < n_particles; i++)
        fprintf(f, "%e\n", module(particle[i].mom) );
    fclose(f);
}

double get_kinetic_energy()
{
    int i,j;
    double sum = 0.0f;
    for(i = 0; i < n_particles; i++)
        for(j = 0; j < DIMENSION; j++)
            sum += particle[i].mom[j] * particle[i].mom[j];
    return sum / 2.0f;
}

double get_temperature()
{
    return (2.0f * get_kinetic_energy() / (DIMENSION * n_particles));
}

double get_total_momentum(){
    int i,j;
    double sum[DIMENSION] = {0.0f};
    for(i = 0; i < n_particles; i++)
        for (j = 0; j < DIMENSION; j++)
            sum[j] += particle[i].mom[j];
    return module(sum);
}

/* rescale all the momenta to set the desired temperature */
void set_temperature(double temp)
{
    int i,j;
    double energy = get_kinetic_energy();
    for(i = 0; i < n_particles; i++)
        for(j = 0; j < DIMENSION; j++)
            particle[i].mom[j] *= sqrt(n_particles*temp/energy);
}


/* compute collision time of the pair (i,j) */
double get_collision_time(int i, int j)
{
    double delta_pos[DIMENSION], delta_mom[DIMENSION];
    double t, rr, rv, vv, delta, collision_time = DBL_MAX;

    int k,dx[2];
    for(dx[0] = -1; dx[0] <= 1 ; dx[0]++)
        for(dx[1] = -1; dx[1] <= 1; dx[1]++){
            vv = rv = rr = 0.0f;
            for(k = 0; k < DIMENSION; k++){
                delta_pos[k] = (1.0f * dx[k]) + particle[j].pos[k] - particle[i].pos[k];
                delta_mom[k] = particle[j].mom[k] - particle[i].mom[k];
                rr += delta_pos[k] * delta_pos[k];
                rv += delta_pos[k] * delta_mom[k];
                vv += delta_mom[k] * delta_mom[k];
            }
            delta = rv*rv-vv*(rr-SIGMA*SIGMA);
            if( (rv < 0)&&(delta > 0) ){
                t = (-rv-sqrt(delta))/vv;
                if(t < collision_time)
                    collision_time = t;
            }
        }
    return collision_time;
}

/* fill collision table */
void get_collision_table()
{
    int i,j;
    for(i = 0; i < n_particles; i++)
        for(j = i+1; j < n_particles; j++)
            collision_table[i*n_particles+j] = get_collision_time(i,j);
}

/* find first pair of colliding particles (to ) */
double get_first_collision()
{
    int i,j;
    double min = DBL_MAX;
    for(i = 0; i < n_particles; i++)
        for(j = i+1; j < n_particles; j++)
            if(collision_table[i*n_particles+j] < min){
                min = collision_table[i*n_particles+j];
                collider[0] = i;
                collider[1] = j;
            }
    return min;
}

/* update collision table */
void update_collision_table()
{
    int i, j;
    for(i = 0; i < n_particles; i++)
        for(j = i+1; j < n_particles; j++)

            /* if one of the particles in the pair (i,j) takes part in the collision, update its row and column */
            if(i == collider[0] || j == collider[0] || i == collider[1] || j == collider[1])
                collision_table[i*n_particles+j] = get_collision_time(i,j);

            /* if particles (i,j) are spectators, just subtract time until the collision */
            else collision_table[i*n_particles+j] -= min_time;
    min_time = 0.0f;
}

void reset_variables()
{
    int i;
    for(i = 0; i < n_particles; i++){
        particle[i].distance = 0.0f;
        particle[i].last_collision_time = 0.0f;
        particle[i].n_collisions = 0;
    }

    idx_history_time = 0;
    n_collisions = 0;
    min_time = 0.0f;
    runtime = 0.0f;
    pressure = 0.0f;
    dp = 0.0f;

    get_collision_table();
}

/* free all the allocated memory */
void clean()
{
    if(particle)        free(particle);
    if(collision_table) free(collision_table);
    if(history)         free(history);
}

/* initialization */
int init(double eta, double temperature)
{
    SIGMA = 1.1283791671 * sqrt( eta/n_particles );

    /* check if disks fit the box */
    int k = 0; while( k * k < n_particles ) k++;
    if( SIGMA > 1.0f / k ){
        printf("Disks are too close to each other!\n");
        return 1;
    }

    /* initialize pseudo-random number generators */
    int i,j;
    srand(time(NULL));
    seed_mersenne( (long)time(NULL) );
    for(i = 0; i < 543210; i++) mersenne();

    /* allocate memory */
    clean();
    collision_table = (double*)malloc( n_particles * n_particles * sizeof(double) );
    particle = (body*)malloc( n_particles * sizeof(body));
    history = (body*)malloc( n_history * n_particles * sizeof(body) );

    /* initialize position and momentum of particles */
    for(i = 0; i < n_particles; i++){
        /* place disks on a square lattice */
        particle[i].pos[0] = (0.5f/k)+(i%k)*(1.0f/k);
        particle[i].pos[1] = (0.5f/k)+(i/k)*(1.0f/k);

        /* set random initial momentum in [-1:1] */
        particle[i].mom[0] = 2.0f * mersenne() - 1.0f;
        particle[i].mom[1] = 2.0f * mersenne() - 1.0f;
    }

    /* compute center of mass momentum */
    double com_momentum[DIMENSION] = {0.0f};
    for(i = 0; i < n_particles; i++)
        for(j = 0; j < DIMENSION; j++)    
            com_momentum[j] += particle[i].mom[j];

    /* boost in the center of mass frame */
    for(i = 0; i < n_particles; i++)
        for(j = 0; j < DIMENSION; j++)
            particle[i].mom[j] -= com_momentum[j] / n_particles;

    set_temperature(temperature);

    /* init stuff */
    reset_variables();
    
    return 0;
}

/* move particles for time_step */
void step(double dt){
    int i,j;
    for(i = 0; i < n_particles; i++)
        for(j = 0; j < DIMENSION; j++){
            particle[i].pos[j] += particle[i].mom[j] * dt;
            particle[i].pos[j] -= floor(particle[i].pos[j]);
        }
}

void run()
{
    min_time = get_first_collision();

    if( min_time < time_step ){

    }
    else{}

    double dr[DIMENSION], dv[DIMENSION], tmp[DIMENSION];

    int i,j;
    /* move each particle and put it back in the first box */
    for(i = 0; i < n_particles; i++)
        for(j = 0; j < DIMENSION; j++)
            particle[i].pos[j] = fmod(fmod( particle[i].pos[j]+particle[i].mom[j]*min_time ,1.0)+1.0,1.0);

    /* compute distance between colliding particles in the same box */
    for(j = 0; j < DIMENSION; j++)
        dr[j] = particle[collider[1]].pos[j] - particle[collider[0]].pos[j];

    /* find the right copy of the particle to collide with */
    double dx[2];
    for(dx[0] = -1; dx[0] <= 1 ; dx[0]++)
        for(dx[1] = -1; dx[1] <= 1; dx[1]++){
            for(j = 0; j < DIMENSION; j++)
                tmp[j] = dx[j] + particle[collider[1]].pos[j] - particle[collider[0]].pos[j];
            if(scalar(tmp,tmp) < scalar(dr,dr))
                for(j = 0; j < DIMENSION; j++)
                    dr[j] = tmp[j];
        }

    /* update data of colliding particles */
    for(i = 0; i < 2; i++){
        particle[collider[i]].distance += (runtime + min_time - particle[collider[i]].last_collision_time) * module(particle[collider[i]].mom);
        particle[collider[i]].last_collision_time = runtime;
        particle[collider[i]].n_collisions++;
    }

    /* update momenta of colliding particles */
    for(j = 0; j < DIMENSION; j++){
        dv[j] = particle[collider[0]].mom[j] - particle[collider[1]].mom[j];
        tmp[j] = scalar(dv,dr)*dr[j]/scalar(dr,dr);
        particle[collider[0]].mom[j] -= tmp[j];
        particle[collider[1]].mom[j] += tmp[j];
    }

    n_collisions++;
    runtime += min_time;
    dp += module(dv);
    pressure = n_particles * get_temperature() * ( 1 + SIGMA * dp / ( 2 * get_kinetic_energy() * runtime ) );

    update_collision_table();
}

/*Rimette le particelle nella scatola*/
void fix_boundaries(){
    int i,j;
    for(i = 0 ; i < n_particles; i++)
        for(j = 0; j < DIMENSION; j++)
            particle[i].pos[j] -= floor(particle[i].pos[j]);
}

/* one step */
void evolve()
{
    double deltaV_pre[DIMENSION];
    double deltaV_post[DIMENSION];
    double deltaV[DIMENSION];

    min_time = get_first_collision();
    /*Mfp da calcolare prima che si siano scambiate le velocità*/
    int i;
    for(i = 0; i < 2; i++){
        particle[collider[i]].n_collisions++;
        particle[collider[i]].last_collision_time = runtime;
        particle[collider[i]].distance += (runtime+min_time-particle[collider[i]].last_collision_time) * module( particle[collider[i]].mom );
    }
    /*
    Ossia:
    time_prec è l'ultimo tempo in cui si son salvati i dati
    runtime è il tempo corrente 
    time_step è la larghezza di step temporale a cui si vuole calcolare dr2
    if ( runtime + min_time <time_prec+time_step ){
    */
    /* Se non ha superato lo step temporale, muovi sempre prendere dati*/
    if ( runtime + min_time - time_step -time_prec < 0){
        step(min_time);
    }
    /* Supererebbe lo step:
    * ~ muovi del tempo necessario per arrivare allo step
    * ~ prende dati
    * ~ muove del tempo necessario per arrivare a min_time
    */
    else{
        step( time_prec + time_step - runtime);

        int j;
        for(j = 0; j < n_particles; j++)
            history[idx_history_time*n_particles+j] = particle[j];
        idx_history_time++;

        step( runtime + min_time - time_prec - time_step);
        time_prec += time_step;
    }
    subtract(particle[collider[0]].mom,particle[collider[1]].mom,deltaV_pre);
/*    switch_moms();*/
    /*calcoli pressure*/
    subtract(particle[collider[0]].mom,particle[collider[1]].mom,deltaV_post);
    subtract(deltaV_pre,deltaV_post,deltaV);
    /*condizioni al bordo*/
    fix_boundaries();
    runtime += min_time;
    update_collision_table();
    n_collisions ++;
    pressure += module(deltaV);
}

/* compute <dr²(dt)> by averaging on every particle at dt fixed. for each particle take the distance with its nearest copy */
double get_dr2(body *list0, body *list1)
{
    int i,j;
    int dx[DIMENSION];
    double distance, sum = 0.0f;
    double dr[DIMENSION];
    for(i = 0; i < n_particles; i++){
        /* first copy */
        for(j = 0; j < DIMENSION; j++)
            dr[j] = list1[i].pos[j] - list0[i].pos[j];
        distance = scalar(dr,dr);
        /* check other copies */
        for(dx[0] = -1; dx[0] <= 1 ; dx[0]++)
            for(dx[1] = -1; dx[1] <= 1; dx[1]++){
                for(j = 0; j < DIMENSION; j++)
                    dr[j] = dx[j] + list1[i].pos[j] - list0[i].pos[j];
                if(scalar(dr,dr) < distance)
                    distance = scalar(dr,dr);
            }
        sum += distance;
    }
    return sum / n_particles;
} 

/* compute <dr²(dt)> for every dt and write to file */
void print_dr2(char *filename)
{
    FILE *f = fopen(filename, "w");
    int count, start, shift;
    double tmp;
    double mean, error;
    for(shift = 0; shift < idx_history_time-1; shift++){
        error = mean = count = 0;
        for(start = 0; (start + shift) < idx_history_time; start++){
            tmp = get_dr2( &history[start * n_particles], &history[(start+shift)*n_particles] );
            mean += tmp;
            error += tmp*tmp;
            count++;
        }
        mean /= count;
        error = sqrt( error / count - mean * mean );
        fprintf(f,"%e\t%e\t%e\n", shift * time_step, mean, error );
    }
    fclose(f);
}

#endif