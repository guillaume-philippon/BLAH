/*
#  File:     BUpdaterPBS.h
#
#  Author:   Massimo Mezzadri
#  e-mail:   Massimo.Mezzadri@mi.infn.it
# 
# Copyright (c) Members of the EGEE Collaboration. 2004. 
# See http://www.eu-egee.org/partners/ for details on the copyright
# holders.  
# 
# Licensed under the Apache License, Version 2.0 (the "License"); 
# you may not use this file except in compliance with the License. 
# You may obtain a copy of the License at 
# 
#     http://www.apache.org/licenses/LICENSE-2.0 
# 
# Unless required by applicable law or agreed to in writing, software 
# distributed under the License is distributed on an "AS IS" BASIS, 
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
# See the License for the specific language governing permissions and 
# limitations under the License.
# 
*/

#include "job_registry.h"
#include "Bfunctions.h"
#include "config.h"
#include "mtsafe_popen.h"

#ifndef VERSION
#define VERSION            "1.8.0"
#endif

int IntStateQuery();
int FinalStateQuery(char *input_string);
int AssignFinalState(char *batchid);

int runfinal=FALSE;
char *pbs_binpath=NULL;
char *pbs_spoolpath=NULL;
char *registry_file;
int purge_interval=864000;
int finalstate_query_interval=30;
int alldone_interval=600;
int next_finalstatequery=0;
int debug=0;
int nodmn=0;

bupdater_active_jobs bact;

FILE *debuglogfile;
char *debuglogname;

job_registry_handle *rha;
config_handle *cha;
config_entry *ret;
char *progname="BUpdaterPBS";

