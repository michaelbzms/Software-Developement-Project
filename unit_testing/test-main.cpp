#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
#include "../Headers/JobScheduler.h"

JobScheduler *scheduler = new JobScheduler();  // this is a leak but for testing purposes it's fine
