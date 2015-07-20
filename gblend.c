/*  &copy; Copyright 2013 by Al Williams (al.williams@awce.com) 
    Distributed under the terms of the GNU General Public License */

/*
    Blend multiple gcode files by layer

    This file is part of GBlend.

    GBlend is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

#ifndef DEBUG
#define DEBUG 0
#endif

void usage()
{
  fprintf(stderr,
	  "gblend by Al Williams V Beta 0.2 - 15 July 2015\n"
"Copyright 2013-2015 by Al Williams (al.williams@awce.com)\n"
    "Distributed under the terms of the GNU General Public License \n"
    "Blend multiple gcode files by layer\n"
"\n"
    "This file is part of GBlend.\n"
"\n"
    "GBlend is free software: you can redistribute it and/or modify\n"
    "it under the terms of the GNU General Public License as published by\n"
    "the Free Software Foundation, either version 3 of the License, or\n"
    "(at your option) any later version.\n"
"\n"
    "This program is distributed in the hope that it will be useful,\n"
    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
    "GNU General Public License for more details.\n"
"\n"
    "You should have received a copy of the GNU General Public License\n"
    "along with this program.  If not, see <http://www.gnu.org/licenses/>.\n\n"

	  "Usage: gblend [-h] [-o output file] [-t] [-s start_token] [-e end_token] [-S] [-E] [startspec] file endspec [[startspec] file [endspec]...]\n"
	  "-h - This message\n"
	  "-o - Output file name (default is stdout)\n"
	  "-t - Stop processing file after end layer found (will not pick up later layers in range)\n"
	  "-s start_token - Set start token (default: %%%%%%GBLEND_START)\n"
	  "-e end_token - Set end token (default: %%%%%%GBLEND_END)\n"
	  "-S - Do not use start token\n"
	  "-E - Do not use end token\n"
	  "startspec - = for copy, [ for start, [mm.m for layer start\n"
	  "endspec - mm.m] to stop on layer (default is start of next file)\n");
  exit(1);
}

// input line
char linebuf[2049];
// holding buffer
char holdbuf[2049];

// default tokens
char *starttoken="%%%GBLEND_START";
char *endtoken="%%%GBLEND_END";


// Processing engine -- this could be called from a GUI
// out - output file (open)
// fn - input file path
// start - starting mm layer
// end - end mm layer
// equal - 1 if end is <=, 0 if <  (mm.m])
// all - 1 if we get the whole file no matter what (=)
// fromstart - 1 if we get from start [
// toend - 1 if we get to end ]
// noterm - Keep looking even after Z overflows (-t option)
// nostarttoken, noendtoken - 1 for -S and -E options

// states are:
// 0 = Look for Z
// 1 = Copy
// 2 = Done
// 3 = Not used
// 4 = Look for start token
int process(FILE *out,char *fn,float start, float end, int equal, int all, int fromstart, int toend, int noterm, int nostarttoken, int noendtoken)
{
  FILE *in=fopen(fn,"r");
  if (!in) return 1;
  int g1flag=0;  // this tells us we are in the middle of an unknown G1
  int state=all?1:4;  // default state is 1 if we get everything or 4 if we are looking for start tag
  if (all) noendtoken=1;  // if all, ignore end token
  if (nostarttoken && state==4) state=1;  // if -S in effect, forget state 4
  // scan each line
  while (fgets(linebuf,sizeof(linebuf),in))
    {
#if DEBUG==1
      printf("Looking at %s\n",linebuf);
#endif      
      // scan each token
      char *token=strtok(linebuf," \t\n");;
      if (!token) continue;
#if DEBUG==1
      printf("Looking at token %s in state %d\n",token,state);
#endif      
      if (state==2) break;   // state 2 means we are done
      do  // for each token
	{
	  if (state==2) break;  // done
	  if (noendtoken==0&&!strcmp(token+(token[0]==';'),endtoken)) 
	    {
	      // if we find an end token flush anything we are holding and go to state 2
	      if (g1flag) fprintf(out,"%s\n",holdbuf);
	      state=2;
#if DEBUG==1
	      printf("Found end token\n");
#endif	      
	      break;
	    }
	  
#if DEBUG==1
	  if (state==4) printf("Looking for start token\n");
#endif	  
	  // if we are looking for start token and we don't have it, keep going
	  if (state==4 && strcmp(token+(token[0]==';'),starttoken)) continue;
	  // if we got here in state 4, we found a start token
	  if (state==4) 
	    {
#if DEBUG==1
	      printf("Found start token. Going to state %d\n",fromstart?1:0);
#endif	      
	      // go to the correct state (1=copy, 0=find start Z)
	      state=fromstart?1:0;
	      continue;
	    }
	  // look for G1 commands
	  // Note: All FW I know of treats G1 and G0 the same
	  // So we convert all input G0 to G1
	  if ((!strcmp(token,"G1"))||(!strcmp(token,"G0"))) 
	    {
	      // flush any previous G1
	      if (g1flag && state==1) fprintf(out,"%s",holdbuf);
	      g1flag=1;
	      strcpy(holdbuf,"G1 ");
	    }
	  else if (g1flag && *token=='Z')   // here we are in a G1 and we found a Z
	    {
	      
	      float z=atof(token+1);
#if DEBUG==1
	      printf("DBG FOUND Z ****** %f\n",z);
	      printf("start=%f end=%f all=%d toend=%d equal=%d\n",start, end, all,toend,equal);
	      
#endif
	      g1flag=0;
	      // see if we need to change state
	      switch (state)
		{
		case 0:  // not printing, do we start?
		  if (z>=start)
		    {
		      if (!toend)
			{
			  if (z>=end && !equal) continue;
			  if (z>end && equal) continue;
			}
		      state=1;
		      fprintf(out,"%s%s ",holdbuf,token);  // print pending 
		    }
		  break;
		case 1:  // printing, do we stop?
		  if (all==0 && toend==0) 
		    {
		      // stop on these
		      if (z>=end && !equal) state=2;
		      if (z>end && equal) state=2;
		      if (state==2 && g1flag) fprintf(out,"%s\n",holdbuf);
		      if (state==2) break;
		    }
		  if (state==1) fprintf(out,"%s%s ",holdbuf,token);
		  // if we are going to stop but noterm is in effect, go back to state 0
		  if (state==2 && noterm) state=0; else break;
		}
	    }
	  else if (g1flag) // if we got another G command flush held command
	    {
	      if (*token=='G')
		{
		  if (state==1) fprintf(out,"%s%s ",holdbuf,token);
		  g1flag=0;
		}
	      else   // otherwise keep holding output in case we get a Z later
		{
		  strcat(holdbuf,token);
		  strcat(holdbuf," ");
		}
	    }
	  else
	    {
	      // if G not 1...
	      if (*token=='G')
		{
		  if (g1flag && state==1) fprintf(out,"%s",holdbuf);  // output hold data
		  g1flag=0;
		}
	      
	      // other things, copy if state=1
	      if (state==1) fprintf(out,"%s ",token);
	    }
	} while ((token=strtok(NULL," \t\n")));
      // print or buffer new line
      if (state==1 && !g1flag) fprintf(out,"\n");
      if (state==1 && g1flag) strcat(holdbuf,"\n");
    }
  // flush anything left
  if (g1flag && state==1) fprintf(out,"%s ",holdbuf);
  // close files
  fclose(in);
  if (out!=stdout) fclose(out);
  return 0;
}

// command line driver
int main(int argc, char *argv[])
{
  unsigned nextlayer=1;
  int c,i, skipflag;
  float start_mm=0.0, end_mm;
  int fromstartflag, allflag, toendflag, endequalflag;
  int term=1,nostart=0,nostop=0;
  char *fn;
  FILE *out=stdout;  // assume stdou
  if (argc<2) usage();
  // parse arguments
  while ((c=getopt(argc,argv,"ho:Ss:Ee:t"))!=-1)
    {
      switch (c)
	{
	case 'o':
	  out=fopen(optarg,"w");
	  if (!out)
	    {
	      perror(optarg);
	      exit(2);
	    }
	  break;
	  
	case 't':
	  term=0;
	  break;
	  
	case 'S':
	  nostart=1;
	  break;
	  
	case 'E':
	  nostop=1;
	  break;

	case 's':
	  nostart=0;
	  starttoken=strdup(optarg);
	  break;
	  
	case 'e':
	  nostop=0;
	  endtoken=strdup(optarg);
	  break;
	  
	  
	case 'h':
	case '?':
	default:
	  usage();
	}
    }

  for (i=optind; i<argc;i++)
    {
      // this is the main "state machine"
      char *p=argv[i];
      // so here we either have a start spec or a file name
      // start spec can be empty at which point it is just
      // current position
      // or it can be
      // [ - from start of file
      // [x.x - From position x.x
      // x.x - From position x.x
      // = take all of file
      fromstartflag=allflag=toendflag=endequalflag=0;
      if (*p=='=')  // take it all
	{
	  allflag=1;
	  if (++i==argc) usage();
	}
      else 
	{
	  if (*p=='[')  // [ take from start of file or from n.n (synonym for n.n)
	    {
	      if (!p[1]) 
		{
		  fromstartflag=1;
		  if (++i==argc) usage();
		}
	      else
		p++;
	    }
	      
	  
	  if (*p=='.' || isdigit(*p)) 
	    {
	      start_mm=atof(p);
	      if (++i==argc) 
		{
		  // this isn't an error
		  // it just means the last file probably
		  // entered an end but no more file
		  continue;
		}
	    }
	}
      // now we better have a file name
      fn=argv[i];
#if DEBUG==1
      printf("File: %s\n",fn);
#endif      
      fprintf(out,"; %%%%%%GBLEND_FILE=%s\n",fn);
      if (i+1!=argc)
	{
	  // peek ahead to see if we have a stop or start for next
	  // if we have x.x] that means go up to that point in this
	  // file including x.x
	  // if the next start is just x.x then we don't include
	  // x.x or greater for this file
	  // or just ] is all the way to end
	  if ((argv[i+1][0])=='.'||isdigit(argv[i+1][0]))
	    {
	      // numeric start
	      // possibly with ] at end
	      if (argv[i+1][strlen(argv[i+1])-1]==']')
		endequalflag=1;
	      end_mm=atof(argv[i+1]);
	      if (endequalflag) i++;  // skip end argument
	    }
	  else if (*argv[i+1]==']' && !argv[i+1][1])
	    {
	      toendflag=1;
	      i++;
	    }
	  else 
	    {
	      //? what?
	    }
	}
      else
	{
	toendflag=1;
	}
      
	  // now we can read the file. If we have all set, we get all
	  // if we have a start set, we wait for start before copy
	  // if we have an end set we stop after it is equal or < depending on flags
      process(out,fn,start_mm,end_mm,endequalflag,allflag,fromstartflag,toendflag,term,nostart,nostop);
      
    }
  return 0;
  
}

