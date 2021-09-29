#ifndef main_h
#define main_h

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include "tui.h"
#include "logging.h"
#include "hstring.h"
#include "communication.h"
#include "array.h"

#define ARRAY_SIZE(arr) (int)(sizeof(arr)/sizeof(arr[0]))

#endif
