#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// pso parameters, change accordingly
#define INERTIA_WEIGHT 0.5
#define PERSONAL_BEST_WEIGHT 2.0
#define GLOBAL_BEST_WEIGHT 2.0

// global values for verbose mode
int verbose = 0;

typedef struct {
    char *password;
    double fitness;
    char *personalBest;
    double personalBestFitness;
    double velocity;
} Particle;

double calculateFitness(const char *password) {
    int length = strlen(password);
    double randomness = 0.0, varietyPenalty = 0.0, repetitionPenalty = 1.0;
    bool hasUppercase = false, hasLowercase = false, hasNumber = false, hasSymbol = false;

    for (int i = 0; i < length - 1; ++i) {
        randomness += abs(password[i + 1] - password[i]);

        // variety check - checks if there are: uppercase, lowercase, number, symbol
        if (isupper(password[i])) {
            hasUppercase = true;
        } else if (islower(password[i])) {
            hasLowercase = true;
        } else if (isdigit(password[i])) {
            hasNumber = true;
        } else {
            hasSymbol = true;
        }

        // adds penalty if there are repeated characters
        if (password[i] == password[i + 1]) {
            repetitionPenalty -= 0.2;
        }

        // adds penalty for the *total* amount of repeated characters
        for (int j = i + 1; j < length; ++j) {
            if (password[i] == password[j]) {
                repetitionPenalty -= 0.05;
            }
        }
    }
    // makes sure repetition penalty isn't lower than 0
    if (repetitionPenalty < 0.0) {
        repetitionPenalty = 0.0;
    }

    // penalty based on variety
    if (hasLowercase) {
        varietyPenalty += 0.25;
    }
    if (hasUppercase) {
        varietyPenalty += 0.25;
    }
    if (hasNumber) {
        varietyPenalty += 0.25;
    }
    if (hasSymbol) {
        varietyPenalty += 0.25;
    }

    return (((double)length + randomness) * varietyPenalty * repetitionPenalty);
}

void initializeParticle(Particle *particle, int length) {
    particle->password = (char *)malloc((length + 1) * sizeof(char));
    for (int i = 0; i < length; ++i) {
        // initialize password with a random printable ascii character
        particle->password[i] = 33 + rand() % 94;
    }
    particle->password[length] = '\0';

    particle->fitness = calculateFitness(particle->password);

    // init pb
    particle->personalBest = strdup(particle->password);
    particle->personalBestFitness = particle->fitness;

    // init vel
    particle->velocity = 0.0;
}

void updateParticle(Particle *particle, const char *globalBest, double inertiaWeight) {
    // references: https://web2.qatar.cmu.edu/~gdicaro/15382/additional/CompIntelligence-Engelbrecht-ch16.pdf
    int length = strlen(particle->password);

    for (int i = 0; i < length; ++i) {
        double r1 = (double)rand() / RAND_MAX;
        double r2 = (double)rand() / RAND_MAX;
        double cognitiveComponent = PERSONAL_BEST_WEIGHT * r1 * (particle->personalBest[i] - particle->password[i]);
        double socialComponent = GLOBAL_BEST_WEIGHT * r2 * (globalBest[i] - particle->password[i]);

        particle->velocity = inertiaWeight * particle->velocity + cognitiveComponent + socialComponent;
        particle->password[i] += round(particle->velocity);

        // make sure character is ascii.
        // however, this causes a problem where most of the time
        // the character that is chosen is '!' or '~'
        if (particle->password[i] < 33) {
            particle->password[i] = 33;
        } else if (particle->password[i] > 126) {
            particle->password[i] = 126;
        }
    }

    particle->fitness = calculateFitness(particle->password);

    // update pb if better
    if (particle->fitness > particle->personalBestFitness) {
        free(particle->personalBest);
        particle->personalBest = strdup(particle->password);
        particle->personalBestFitness = particle->fitness;
    }
}

int findBestParticle(Particle *swarm, int numParticles) {
    int bestIndex = 0;

    for (int i = 1; i < numParticles; ++i) {
        if (swarm[i].fitness > swarm[bestIndex].fitness) {
            bestIndex = i;
        }
    }

    return bestIndex;
}

void freeParticles(Particle *swarm, int numParticles) {
    for (int i = 0; i < numParticles; ++i) {
        free(swarm[i].password);
        free(swarm[i].personalBest);
    }
}

char *generatePassword(int length, int numParticles, int maxIterations) {
    Particle *swarm = (Particle *)malloc(numParticles * sizeof(Particle));

    // initialize all particles
    for (int i = 0; i < numParticles; ++i) {
        initializeParticle(&swarm[i], length);
    }

    // initial gb
    int globalBestIndex = findBestParticle(swarm, numParticles);
    char *globalBest = strdup(swarm[globalBestIndex].password);

    // start pso
    for (int iteration = 0; iteration < maxIterations; ++iteration) {
        double inertiaWeight = INERTIA_WEIGHT - (INERTIA_WEIGHT / maxIterations) * iteration;

        for (int i = 0; i < numParticles; ++i) {
            updateParticle(&swarm[i], globalBest, inertiaWeight);
        }

        // update gb
        globalBestIndex = findBestParticle(swarm, numParticles);
        free(globalBest);
        globalBest = strdup(swarm[globalBestIndex].password);

        if (verbose) {
            printf("Iteration %d - Best Fitness = %.1f\n", iteration + 1, swarm[globalBestIndex].fitness);
            printf("globalBest: %s\n", globalBest);
            printf("------------------------------\n");
            for (int i = 0; i < numParticles; ++i) {
                printf("Particle %.2d: %s \t(fitness: %.1f)\n", i + 1, swarm[i].password, swarm[i].fitness);
            }
            printf("\n");
        }
    }

    // free memory
    freeParticles(swarm, numParticles);
    free(swarm);

    return globalBest;
}

int main() {
    srand(time(NULL));

    // set default user parameters
    int length = 16;
    int numParticles = 10;
    int maxIterations = 100;

    // option to not use default
    char option = {'\0'};
    printf("Use default parameters? (y/n) ");
    do {
        scanf("%c", &option);
        option = tolower(option);
    } while (option != 'y' && option != 'n');

    // if not default
    if (option == 'n') {
        printf("Password length (default = 16): ");
        scanf("%d", &length);
        getchar();

        printf("Number of particles (default = 10): ");
        scanf("%d", &numParticles);
        getchar();

        printf("Maximum iterations (default = 100): ");
        scanf("%d", &maxIterations);
        getchar();

        printf("\n");
    }

    do {
        printf("Verbose mode? (0/1) ");
        scanf("%d", &verbose);
        getchar();
    } while (verbose != 0 && verbose != 1);

    char *password = generatePassword(length, numParticles, maxIterations);
    printf("Generated Password: %s\n", password);

    // free memory
    free(password);

    return 0;
}
