#ifndef PD_H
#define PD_H

#include <jni.h>
#include <jvmti.h>

int pd_init(char *n, unsigned int p, unsigned int c);
int pd_finalize();
void pd_get_errorstr(char *buf, size_t buf_size);

void commander_entry(jvmtiEnv *jvmti, JNIEnv *env, void *arg);

#endif
