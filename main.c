#ifdef _WINDOWS
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#endif

#include <jvmti.h>
#include <jni.h>

#include <string.h>
#include <stdlib.h>

#include "pd.h"

#define COMMANDER_THREAD_NAME "stdlog commander"

typedef struct{
  char *file;
  unsigned short port;
  unsigned int count;
} TOptions;

static TOptions opt;


static void parse_options(char *options){
  char *key, *value;

  /* Set default values */
  memset(&opt, 0, sizeof(TOptions));
  opt.count = 5;

  /* Parse values from option string */
  while((key = strtok(options, ",")) != NULL){
    value = strchr(key, '=');
    *value = '\0';
    value++;

    if(strcmp(key, "file") == 0){
      opt.file = strdup(value);
    }
    else if(strcmp(key, "port") == 0){
      sscanf(value, "%hu", &opt.port);
    }
    else if(strcmp(key, "count") == 0){
      sscanf(value, "%u", &opt.count);
    }
    else{
      fprintf(stderr, "stdlog: Unknown option: %s\n", key);
    }

    options = NULL;
  }

}

void JNICALL OnVMInit(jvmtiEnv *jvmti, JNIEnv* env, jthread thread){
  jclass thread_cls;
  jmethodID constructor_id;
  jstring thread_name_str;
  jthread commander_thread;
  jvmtiError err;

  thread_cls = (*env)->FindClass(env, "java/lang/Thread");
  constructor_id = (*env)->GetMethodID(env, thread_cls,
                                         "<init>", "(Ljava/lang/String;)V");

  thread_name_str = (*env)->NewStringUTF(env, COMMANDER_THREAD_NAME);
  if(thread_name_str == NULL){

    if(!(*env)->ExceptionCheck(env)){
      jclass npe = (*env)->FindClass(env, "java/lang/NullPointerException");
      (*env)->ThrowNew(env, npe, "stdlog: NewString() failed.");
    }

    (*env)->ExceptionDescribe(env);
    return;
  }

  commander_thread = (*env)->NewObject(env, thread_cls,
                                         constructor_id, thread_name_str);
  if(commander_thread == NULL){

    if(!(*env)->ExceptionCheck(env)){
      jclass npe = (*env)->FindClass(env, "java/lang/NullPointerException");
      (*env)->ThrowNew(env, npe, "stdlog: NewObject() failed.");
    }

    (*env)->ExceptionDescribe(env);
    return;
  }

  err = (*jvmti)->RunAgentThread(jvmti, commander_thread, &commander_entry,
                                              NULL, JVMTI_THREAD_NORM_PRIORITY);
  if(err != JVMTI_ERROR_NONE){
    char *jvmti_error;
    (*jvmti)->GetErrorName(jvmti, err, &jvmti_error);
    fprintf(stderr, "stdlog: RunAgentThread(): %s\n", jvmti_error);
    fprintf(stderr, "stdlog: commander thread could not  be started.\n");
    (*jvmti)->Deallocate(jvmti, jvmti_error);
  }

}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved){
  jvmtiEnv *jvmti;
  jvmtiEventCallbacks callback;
  jvmtiError err;

  if((*vm)->GetEnv(vm, (void **)&jvmti, JVMTI_VERSION_1) != JNI_OK){
    fprintf(stderr, "stdlog: Could not get jvmtiEnv\n");
    return JNI_ERR;
  }

  if(options == NULL){
    fprintf(stderr, "stdlog: file option must be set\n");
    return JNI_ERR;
  }

  parse_options(options);

  if(opt.file == NULL){
    fprintf(stderr, "stdlog: file option must be set\n");
    return JNI_ERR;
  }

  printf("stdlog: logfile = %s\n", opt.file);
  printf("stdlog: port = %d\n", opt.port);
  printf("stdlog: count = %d\n", opt.count);

  memset(&callback, 0, sizeof(jvmtiEventCallbacks));
  callback.VMInit = &OnVMInit;

  err = (*jvmti)->SetEventCallbacks(jvmti, &callback,
                                               sizeof(jvmtiEventCallbacks));
  if(err != JVMTI_ERROR_NONE){
    char *buf;
    (*jvmti)->GetErrorName(jvmti, err, &buf);
    fprintf(stderr, "stdlog: %d @ %s: %s\n", __LINE__, __FILE__, buf);
    (*jvmti)->Deallocate(jvmti, buf);
    return JNI_ERR;
  }

  err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
                                               JVMTI_EVENT_VM_INIT, NULL);
  if(err != JVMTI_ERROR_NONE){
    char *buf;
    (*jvmti)->GetErrorName(jvmti, err, &buf);
    fprintf(stderr, "stdlog: %d @ %s: %s\n", __LINE__, __FILE__, buf);
    (*jvmti)->Deallocate(jvmti, buf);
    return JNI_ERR;
  }

  return (pd_init(opt.file, opt.port, opt.count) == 0) ? JNI_OK : JNI_ERR;
}

JNIEXPORT void JNICALL Agent_OnUnload(JavaVM *vm){

  if(pd_finalize() != 0){
    perror("stdlog");
  }

  free(opt.file);
}

