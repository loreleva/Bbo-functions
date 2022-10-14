
#ifndef _bbcomplib_H_
#define _bbcomplib_H_


#include <assert.h>


#ifdef __cplusplus
extern "C" {
#endif


#define stringtype char const*

int loadProblems(stringtype problemfile, stringtype tracksfile);
int numberOfTracks();
stringtype trackName(int trackindex);
int setTrack(stringtype trackname);
int numberOfProblems();
int setProblem(int problemID);
int dimension();
int numberOfObjectives();
int budget();
int evaluations();
int evaluate(double* point, double* value);
double performance();
stringtype errorMessage();


#ifdef __cplusplus
} // extern "C"
#endif


#endif
