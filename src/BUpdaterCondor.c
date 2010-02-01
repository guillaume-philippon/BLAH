/*
#  File:     BUpdaterCondor.c
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

#include "BUpdaterCondor.h"

extern int bfunctions_poll_timeout;

int main(int argc, char *argv[]){

	FILE *fd;
	job_registry_entry *en;
	time_t now;
	time_t purge_time=0;
	char *constraint=NULL;
	char *query=NULL;
	char *q=NULL;
	char *pidfile=NULL;
	
	poptContext poptcon;
	int rc=0;			     
	int version=0;
	int qlen=0;
	int first=TRUE;
        int tmptim=0;
	int loop_interval=DEFAULT_LOOP_INTERVAL;
	
	struct poptOption poptopt[] = {     
		{ "nodaemon",      'o', POPT_ARG_NONE,   &nodmn, 	    0, "do not run as daemon",    NULL },
		{ "version",       'v', POPT_ARG_NONE,   &version,	    0, "print version and exit",  NULL },
		POPT_AUTOHELP
		POPT_TABLEEND
	};
		
	argv0 = argv[0];

        signal(SIGHUP,sighup);

	poptcon = poptGetContext(NULL, argc, (const char **) argv, poptopt, 0);
 
	if((rc = poptGetNextOpt(poptcon)) != -1){
		sysfatal("Invalid flag supplied: %r");
	}
	
	poptFreeContext(poptcon);
	
	if(version) {
		printf("%s Version: %s\n",progname,VERSION);
		exit(EXIT_SUCCESS);
	}   

        /* Checking configuration */
        check_config_file();

	cha = config_read(NULL);
	if (cha == NULL)
	{
		fprintf(stderr,"Error reading config: ");
		perror("");
		return -1;
	}

	ret = config_get("bupdater_child_poll_timeout",cha);
	if (ret != NULL){
		tmptim=atoi(ret->value);
		if (tmptim > 0) bfunctions_poll_timeout = tmptim*1000;
	}

	ret = config_get("bupdater_debug_level",cha);
	if (ret != NULL){
		debug=atoi(ret->value);
	}
	
	ret = config_get("bupdater_debug_logfile",cha);
	if (ret != NULL){
		debuglogname=strdup(ret->value);
                if(debuglogname == NULL){
                        sysfatal("strdup failed for debuglogname in main: %r");
                }
	}
	if(debug <=0){
		debug=0;
	}
    
	if(debuglogname){
		if((debuglogfile = fopen(debuglogname, "a+"))==0){
			debug = 0;
		}
	}else{
		debug = 0;
	}
	
        ret = config_get("condor_binpath",cha);
        if (ret == NULL){
		do_log(debuglogfile, debug, 1, "%s: key condor_binpath not found\n",argv0);
        } else {
                condor_binpath=strdup(ret->value);
                if(condor_binpath == NULL){
                        sysfatal("strdup failed for condor_binpath in main: %r");
                }
        }
	
	ret = config_get("job_registry",cha);
	if (ret == NULL){
		do_log(debuglogfile, debug, 1, "%s: key job_registry not found\n",argv0);
		sysfatal("job_registry not defined. Exiting");
	} else {
		registry_file=strdup(ret->value);
                if(registry_file == NULL){
                        sysfatal("strdup failed for registry_file in main: %r");
                }
	}
	
	ret = config_get("purge_interval",cha);
	if (ret == NULL){
		do_log(debuglogfile, debug, 1, "%s: key purge_interval not found using the default:%d\n",argv0,purge_interval);
	} else {
		purge_interval=atoi(ret->value);
	}
	
	ret = config_get("finalstate_query_interval",cha);
	if (ret == NULL){
		do_log(debuglogfile, debug, 1, "%s: key finalstate_query_interval not found using the default:%d\n",argv0,finalstate_query_interval);
	} else {
		finalstate_query_interval=atoi(ret->value);
	}
	
	ret = config_get("alldone_interval",cha);
	if (ret == NULL){
		do_log(debuglogfile, debug, 1, "%s: key alldone_interval not found using the default:%d\n",argv0,alldone_interval);
	} else {
		alldone_interval=atoi(ret->value);
	}
	
	ret = config_get("bupdater_loop_interval",cha);
	if (ret == NULL){
		do_log(debuglogfile, debug, 1, "%s: key bupdater_loop_interval not found using the default:%d\n",argv0,loop_interval);
	} else {
		loop_interval=atoi(ret->value);
	}

	ret = config_get("bupdater_pidfile",cha);
	if (ret == NULL){
		do_log(debuglogfile, debug, 1, "%s: key bupdater_pidfile not found\n",argv0);
	} else {
		pidfile=strdup(ret->value);
                if(pidfile == NULL){
                        sysfatal("strdup failed for pidfile in main: %r");
                }
	}
	
	if( !nodmn ) daemonize();


	if( pidfile ){
		writepid(pidfile);
		free(pidfile);
	}
	
	config_free(cha);
	
	rha=job_registry_init(registry_file, BY_BATCH_ID);
	if (rha == NULL){
		do_log(debuglogfile, debug, 1, "%s: Error initialising job registry %s\n",argv0,registry_file);
		fprintf(stderr,"%s: Error initialising job registry %s :",argv0,registry_file);
		perror("");
	}

	for(;;){
		/* Purge old entries from registry */
		now=time(0);
		if(now - purge_time > 86400){
			if(job_registry_purge(registry_file, now-purge_interval,0)<0){
				do_log(debuglogfile, debug, 1, "%s: Error purging job registry %s\n",argv0,registry_file);
                        	fprintf(stderr,"%s: Error purging job registry %s :",argv0,registry_file);
                        	perror("");

			}else{
				purge_time=time(0);
			}
		}	       

		IntStateQuery();
		
		fd = job_registry_open(rha, "r");
		if (fd == NULL)
		{
			do_log(debuglogfile, debug, 1, "%s: Error opening job registry %s\n",argv0,registry_file);
			fprintf(stderr,"%s: Error opening job registry %s :",argv0,registry_file);
			perror("");
			sleep(loop_interval);
			continue;
		}
		if (job_registry_rdlock(rha, fd) < 0)
		{
			do_log(debuglogfile, debug, 1, "%s: Error read locking job registry %s\n",argv0,registry_file);
			fprintf(stderr,"%s: Error read locking job registry %s :",argv0,registry_file);
			perror("");
			sleep(loop_interval);
			continue;
		}
		job_registry_firstrec(rha,fd);
		fseek(fd,0L,SEEK_SET);

		first=TRUE;
		
		while ((en = job_registry_get_next(rha, fd)) != NULL){

			if(en->status!=REMOVED && en->status!=COMPLETED){
			
				if(now-en->mdate>finalstate_query_interval){
					/* create the constraint that will be used in condor_history command in FinalStateQuery*/
					if(!first) strcat(query," ||");	
					if(first) first=FALSE;
					constraint=make_message(" ClusterId==%s",en->batch_id);
					
					if (query != NULL) qlen = strlen(query);
					else               qlen = 0;
					q=realloc(query,qlen+strlen(constraint)+4);
					
					if(q != NULL){
						if (query != NULL) strcat(q,constraint);
						else               strcpy(q,constraint);
						query=q;	
					}else{
						sysfatal("can't realloc query: %r");
					}
					free(constraint);
					runfinal=TRUE;
				}
				
				/* Assign Status=4 and ExitStatus=-1 to all entries that after alldone_interval are still not in a final state(3 or 4)*/
				if(now-en->mdate>alldone_interval && !runfinal){
					AssignFinalState(en->batch_id);	
					free(en);
					continue;
				}
			}
			free(en);
		}
		
		if(runfinal){
			FinalStateQuery(query);
			runfinal=FALSE;
		}
		if (query != NULL){
			free(query);
			query = NULL;
		}
		fclose(fd);		
		sleep(loop_interval);
	}
	
	job_registry_destroy(rha);
	
	return 0;
	
}


int
IntStateQuery()
{
/*
 Output format for status query for unfinished jobs for condor:
 batch_id   user      status     executable     exitcode   udate(timestamp_for_current_status)
 22018     gliteuser  2          /bin/sleep     0          1202288920

 Filled entries:
 batch_id
 status
 exitcode
 udate
 
 Filled by suhmit script:
 blah_id 
 
 Unfilled entries:
 wn_addr
 exitreason
*/

        FILE *fp;
	char *line=NULL;
	char **token;
	int maxtok_t=0;
	job_registry_entry en;
	int ret=0;
	char *cp=NULL; 
	char *command_string=NULL;
	job_registry_entry *ren=NULL;

	command_string=make_message("%s/condor_q -format \"%%d \" ClusterId -format \"%%s \" Owner -format \"%%d \" JobStatus -format \"%%s \" Cmd -format \"%%s \" ExitStatus -format \"%%s\\n\" EnteredCurrentStatus|grep -v condorc-",condor_binpath);
	do_log(debuglogfile, debug, 1, "%s: command_string in IntStateQuery:%s\n",argv0,command_string);
	fp = popen(command_string,"r");

	if(fp!=NULL){
		while(!feof(fp) && (line=get_line(fp))){
			if(line && (strlen(line)==0 || strncmp(line,"JOBID",5)==0)){
				free(line);
				continue;
			}
			if ((cp = strrchr (line, '\n')) != NULL){
				*cp = '\0';
			}
	
			maxtok_t = strtoken(line, ' ', &token);
			if (maxtok_t < 6){
				freetoken(&token,maxtok_t);
				free(line);
				continue;
			}
		
			JOB_REGISTRY_ASSIGN_ENTRY(en.batch_id,token[0]);
			en.status=atoi(token[2]);
			en.exitcode=atoi(token[4]);
			en.udate=atoi(token[5]);
			JOB_REGISTRY_ASSIGN_ENTRY(en.wn_addr,"\0");
			JOB_REGISTRY_ASSIGN_ENTRY(en.exitreason,"\0");
			
			if ((ren=job_registry_get(rha, en.batch_id)) == NULL){
					fprintf(stderr,"Get of record returns error for %s ",en.batch_id);
					perror("");
			}
				
			if(en.status!=UNDEFINED && (en.status!=IDLE || (en.status==IDLE && ren && ren->status==HELD)) && ren && (en.status!=ren->status)){	
				if ((ret=job_registry_update_recn(rha, &en, ren->recnum)) < 0){
					if(ret != JOB_REGISTRY_NOT_FOUND){
						fprintf(stderr,"Update of record returns %d: ",ret);
						perror("");
					}
				} else {
					do_log(debuglogfile, debug, 2, "%s: registry update in IntStateQuery for: jobid=%s creamjobid=%s wn=%s status=%d\n",argv0,en.batch_id,en.user_prefix,en.wn_addr,en.status);
					if (en.status == REMOVED || en.status == COMPLETED)
						job_registry_unlink_proxy(rha, &en);
				}
			}
		
			freetoken(&token,maxtok_t);
			free(line);
			free(ren);
		}
		pclose(fp);
	}

	free(command_string);
	return 0;
}

int
FinalStateQuery(char *query)
{
/*
 Output format for status query for finished jobs for condor:
 batch_id   user      status     executable     exitcode   udate(timestamp_for_current_status)
 22018     gliteuser  4          /bin/sleep     0          1202288920

 Filled entries:
 batch_id
 status
 exitcode
 udate
 
 Filled by suhmit script:
 blah_id 
 
 Unfilled entries:
 wn_addr
 exitreason
*/
        FILE *fp;
	char *line=NULL;
	char **token;
	int maxtok_t=0;
	job_registry_entry en;
	int ret=0;
	char *cp=NULL; 
	char *command_string=NULL;

	command_string=make_message("%s/condor_history -constraint \"%s\" -format \"%%d \" ClusterId -format \"%%s \" Owner -format \"%%d \" JobStatus -format \"%%s \" Cmd -format \"%%s \" ExitStatus -format \"%%s\\n\" EnteredCurrentStatus",condor_binpath,query);
	do_log(debuglogfile, debug, 1, "%s: command_string in FinalStateQuery:%s\n",argv0,command_string);
	fp = popen(command_string,"r");

	if(fp!=NULL){
		while(!feof(fp) && (line=get_line(fp))){
			if(line && (strlen(line)==0 || strncmp(line,"JOBID",5)==0)){
				free(line);
				continue;
			}
			if ((cp = strrchr (line, '\n')) != NULL){
				*cp = '\0';
			}
			
			maxtok_t = strtoken(line, ' ', &token);
			if (maxtok_t < 6){
				freetoken(&token,maxtok_t);
				free(line);
				continue;
			}
		
			JOB_REGISTRY_ASSIGN_ENTRY(en.batch_id,token[0]);
			en.status=atoi(token[2]);
			en.exitcode=atoi(token[4]);
			en.udate=atoi(token[5]);
                	JOB_REGISTRY_ASSIGN_ENTRY(en.wn_addr,"\0");
                	JOB_REGISTRY_ASSIGN_ENTRY(en.exitreason,"\0");
		
			if(en.status!=UNDEFINED && en.status!=IDLE){	
				if ((ret=job_registry_update(rha, &en)) < 0){
					if(ret != JOB_REGISTRY_NOT_FOUND){
						fprintf(stderr,"Update of record returns %d: ",ret);
						perror("");
					}
				} else {
					do_log(debuglogfile, debug, 2, "%s: registry update in FinalStateQuery for: jobid=%s creamjobid=%s wn=%s status=%d\n",argv0,en.batch_id,en.user_prefix,en.wn_addr,en.status);
					if (en.status == REMOVED || en.status == COMPLETED)
						job_registry_unlink_proxy(rha, &en);
				}
			}
			freetoken(&token,maxtok_t);
			free(line);
		}
		pclose(fp);
	}

	free(command_string);
	return 0;
}

int AssignFinalState(char *batchid){

	job_registry_entry en;
	int ret,i;
	time_t now;

	now=time(0);
	
	JOB_REGISTRY_ASSIGN_ENTRY(en.batch_id,batchid);
	en.status=COMPLETED;
	en.exitcode=999;
	en.udate=now;
	JOB_REGISTRY_ASSIGN_ENTRY(en.wn_addr,"\0");
	JOB_REGISTRY_ASSIGN_ENTRY(en.exitreason,"\0");
		
	if ((ret=job_registry_update(rha, &en)) < 0){
		if(ret != JOB_REGISTRY_NOT_FOUND){
			fprintf(stderr,"Update of record %d returns %d: ",i,ret);
			perror("");
		}
	} else {
		do_log(debuglogfile, debug, 2, "%s: registry update in AssignStateQuery for: jobid=%s creamjobid=%s status=%d\n",argv0,en.batch_id,en.user_prefix,en.status);
		job_registry_unlink_proxy(rha, &en);
	}
	
	return 0;
}

void sighup()
{
        if(debug){
                fclose(debuglogfile);
                if((debuglogfile = fopen(debuglogname, "a+"))==0){
                        debug = 0;
                }
        }
}
