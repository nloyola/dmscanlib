/*****************************************************************************
 *
 * Name: $Id: UaDebug.cpp,v 1.3 2007/07/06 22:59:29 octopus Exp $
 *
 *-----------------------------------------------------------------------------
 *
 * The information contained herein is proprietary and confidential to Alberta
 * Ingenuity Centre For Machine Learning (AICML) and describes aspects of AICML
 * products and services that must not be used or implemented by a third party
 * without first obtaining a license to use or implement.
 *
 * Copyright 2006 Alberta Ingenuity Centre For Machine Learning.
 *
 *-----------------------------------------------------------------------------
 *
 * Modification History:
 *
 * Date           Name             Description
 * --------       ---------------  --------------------------------------------
 * Oct25/2006     N.Loyola         Added this comment header.
 *
 *****************************************************************************/

#include "UaDebug.h"

#ifdef WIN32
#include <time.h>
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <fstream>
#include <string>
#include <sstream>

#ifdef UA_HAVE_DEBUG

using namespace std;
using namespace ua;

DebugStringBuf<char> ua::debugstream;
std::ostream ua::cdebug (&ua::debugstream);

DebugImpl::DebugImpl() {
    std::ostringstream header;

    for (unsigned i = 0; i < maxSubSys_m; ++i) {
        levels_am[i] = 0;
        header << "SYS_" << i;
        headers_am[i] = header.str();
        header.str("");
    }
}

void DebugImpl::levelInc(unsigned subsys) {
    assert(subsys <= allSubSys_m);

    if (subsys == allSubSys_m) {
        for (unsigned i = 0; i < maxSubSys_m; ++i) {
            if (levels_am[i] < maxLevel_m)
                ++levels_am[i];
        }
        return;
    }

    if (levels_am[subsys] < maxLevel_m)
        ++levels_am[subsys];
}

void DebugImpl::levelDec(unsigned subsys) {
    assert(subsys <= allSubSys_m);

    if (subsys == allSubSys_m) {
        for (unsigned i = 0; i < maxSubSys_m; ++i) {
            if (levels_am[i] > 0)
                --levels_am[i];
        }
        return;
    }

    if (levels_am[subsys] > 0)
        --levels_am[subsys];
}

void DebugImpl::levelSet(unsigned subsys, unsigned level) {
    assert(subsys <= allSubSys_m);
    assert(level <= maxLevel_m);

    if (subsys == allSubSys_m) {
        for (unsigned i = 0; i < maxSubSys_m; ++i) {
            levels_am[i] = level;
        }
        return;
    }

    if (levels_am[subsys] < maxLevel_m)
        levels_am[subsys] = level;
}

unsigned DebugImpl::levelGet(unsigned subsys) {
    assert(subsys < allSubSys_m);
    return levels_am[subsys];
}

void DebugImpl::subSysHeaderSet(unsigned subsys, std::string header) {
    assert(subsys < allSubSys_m);
    headers_am[subsys] = header;
}

std::string & DebugImpl::subSysHeaderGet(unsigned subsys) {
    assert(subsys < allSubSys_m);
    return headers_am[subsys];
}


// Turns off all debugging
void DebugImpl::reset() {
    for (unsigned i = 0; i < maxSubSys_m; ++i) {
        levels_am[i] = 0;
    }
}

// Returns true if this level is enabled for debugging.
bool DebugImpl::isDebug (unsigned subsys, unsigned level) {
    assert(subsys < allSubSys_m);
    return (level <= levels_am[subsys]);
}

void DebugSink::standardHeader(std::string & str_r) {
    char buf_a[100];

#if defined(WIN32) && defined(__MINGW32__)
    time_t now = time(NULL);
    struct tm *tm_ptr = localtime(&now);
    snprintf(buf_a, sizeof (buf_a), "%02d:%02d:%02d:000 ",
             tm_ptr->tm_hour, tm_ptr->tm_min, tm_ptr->tm_sec);
#elif defined(WIN32)
   time_t ltime;
   struct _timeb tstruct;
   char timebuf[26];
   errno_t err;

   time( &ltime );
   err = ctime_s(timebuf, 26, &ltime);
   if (err) {
      cerr << "ctime_s failed due to an invalid argument.";
      exit(1);
   }
   _ftime_s( &tstruct );
   sprintf_s(buf_a, "%.8s:%03u ", timebuf + 11, tstruct.millitm);
#else
    // Fetch the current time
    char time_a[100];
    struct timeval thistime;

    gettimeofday(&thistime, NULL);
    strftime(time_a, sizeof(time_a), "%X", localtime(&thistime.tv_sec));
    snprintf(buf_a, sizeof (buf_a), "%s:%03ld ", time_a,
             thistime.tv_usec / 1000);
#endif

    str_r = buf_a;
}

void DebugSinkStdoutImpl::write(const std::string& str_r) {
    if (str_r.size() == 0)
        return;

    std::string op;

    if (enableHeader_m)
        header(op);

    op += str_r;

    // Add a trailing newline if we don't have one
    // (we need this when we shut down)
    if (op[op.length()-1] != '\n')
        op += '\n';

    std::cout << op;
}

void DebugSinkFileImpl::write(const std::string& str_r) {
    if (file_m.size() == 0)
        return;

    if (str_r.size() == 0)
        return;

    std::string op;

    if (enableHeader_m)
        header(op);

    op += str_r;

    // Open the file in append mode. The dtor will close
    // the file for us.
    std::ofstream output(file_m.c_str(), std::ios_base::app);
    if (!output)
        return;    // The file could not be opened. Exit

    output << op;
}


void DebugSinkFileImpl::setFile(const std::string& file_r) {
    file_m = file_r;
}

#endif /* DEBUG */