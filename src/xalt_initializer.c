#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>
#include <time.h>

#define PATH_MAX 4096
static char   uuid_str[37];
static double start_time = 0.0;
static double end_time   = 0.0;
static long   my_rank    = 0L;
static long   my_size    = 1L;
static char   path[PATH_MAX];

long compute_size(const char **envA)
{
  long          sz = 0L;
  const char ** p;
  for (p = &envA[0]; *p; ++p)
    {
      char *v = getenv(*p);
      if (v)
        sz += strtol(v, (char **) NULL, 10);
    }

  return sz;
}

void myinit(int argc, char **argv, char **envp)
{
  char * cmdline;
  const char *  rankA[] = {"PMI_RANK", "OMPI_COMM_WORLD_RANK", "MV2_COMM_WORLD_RANK", NULL}; 
  const char *  sizeA[] = {"PMI_SIZE", "OMPI_COMM_WORLD_SIZE", "MV2_COMM_WORLD_SIZE", NULL}; 
  const char ** p;
  struct timeval tv;

  uuid_t uuid;

  char * v = getenv("XALT_EXECUTABLE_TRACKING");
  if (! v)
    return;

  my_rank = compute_size(rankA);
  if (my_rank > 0L)
    return;

  my_size = compute_size(sizeA);
  if (my_size < 1L)
    my_size = 1L;

  if (argv[0][0] == '/')
    strcpy(path,argv[0]);
  else
    {
      int len;
      getcwd(path, PATH_MAX);
      len = strlen(path);
      path[len] = '/';
      strcpy(&path[len+1], argv[0]);
    }
  

  uuid_generate(uuid);
  uuid_unparse_lower(uuid,uuid_str);
  gettimeofday(&tv,NULL);
  start_time = tv.tv_sec + 1.e-6*tv.tv_usec;

  
  asprintf(&cmdline, "LD_LIBRARY_PATH=%s PATH= %s -E %s --start \"%.3f\" --end 0 --uuidgen \"%s\" -- '[{\"exec_prog\": \"%s\", \"ntask\": %ld}]",
	   "@sys_ld_lib_path@", "@python@","@PREFIX@/libexec/xalt_run_submission.py", start_time, uuid_str, path, my_size);

  printf("cmd: %s\n\n",cmdline);
  free(cmdline);
}

void myfini()
{
  char * cmdline;
  struct timeval tv;

  char * v = getenv("XALT_EXECUTABLE_TRACKING");
  if (! v)
    return;

  if (my_rank > 0L)
    return;

  gettimeofday(&tv,NULL);
  end_time = tv.tv_sec + 1.e-6*tv.tv_usec;

  asprintf(&cmdline, "LD_LIBRARY_PATH=%s PATH= %s -E %s --start \"%.3f\" --end \"%.3f\" --uuidgen \"%s\" -- '[{\"exec_prog\": \"%s\", \"ntask\": %ld}]",
	  "@sys_ld_lib_path@", "@python@","@PREFIX@/libexec/xalt_run_submission.py", start_time, end_time, uuid_str, path, my_size);
  printf("cmd: %s\n\n",cmdline);
  free(cmdline);
}

#ifdef __MACH__
  __attribute__((section("__DATA,__mod_init_func"), used, aligned(sizeof(void*)))) typeof(myinit) *__init = myinit;
  __attribute__((section("__DATA,__mod_term_func"), used, aligned(sizeof(void*)))) typeof(myfini) *__fini = myfini;
#else
  __attribute__((section(".init_array"))) typeof(myinit) *__init = myinit;
  __attribute__((section(".fini_array"))) typeof(myfini) *__fini = myfini;
#endif
