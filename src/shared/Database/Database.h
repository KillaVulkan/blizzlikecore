/*
 * Copyright (C) 2013  BlizzLikeGroup
 * BlizzLikeCore integrates as part of this file: CREDITS.md and LICENSE.md
 */

#ifndef DATABASE_H
#define DATABASE_H

#include "Threading.h"
#include "Utilities/UnorderedMap.h"
#include "Database/SqlDelayThread.h"
#include "Policies/Singleton.h"
#include "ace/Thread_Mutex.h"
#include "ace/Guard_T.h"

#ifdef WIN32
  #define FD_SETSIZE 1024
  #include <winsock2.h>
#endif
#include <mysql.h>

class SqlTransaction;
class SqlResultQueue;
class SqlQueryHolder;

typedef UNORDERED_MAP<ACE_Based::Thread*, SqlTransaction*> TransactionQueues;
typedef UNORDERED_MAP<ACE_Based::Thread*, SqlResultQueue*> QueryQueues;

#define MAX_QUERY_LEN   1024

class Database
{
    protected:
        TransactionQueues m_tranQueues;                     // Transaction queues from diff. threads
        QueryQueues m_queryQueues;                          // Query queues from diff threads
        SqlDelayThread* m_threadBody;                       // Pointer to delay sql executer (owned by m_delayThread)
        ACE_Based::Thread* m_delayThread;                   // Pointer to executer thread

    public:

        Database();
        ~Database();

        /*! infoString should be formated like hostname;username;password;database. */
        bool Initialize(const char *infoString);

        void InitDelayThread();
        void HaltDelayThread();

        QueryResult_AutoPtr Query(const char *sql);
        QueryResult_AutoPtr PQuery(const char *format,...) ATTR_PRINTF(2,3);
        QueryNamedResult* QueryNamed(const char *sql);
        QueryNamedResult* PQueryNamed(const char *format,...) ATTR_PRINTF(2,3);

        // Async queries and query holders, implemented in DatabaseImpl.h

        // Query / member
        template<class Class>
            bool AsyncQuery(Class *object, void (Class::*method)(QueryResult_AutoPtr), const char *sql);
        template<class Class, typename ParamType1>
            bool AsyncQuery(Class *object, void (Class::*method)(QueryResult_AutoPtr, ParamType1), ParamType1 param1, const char *sql);
        template<class Class, typename ParamType1, typename ParamType2>
            bool AsyncQuery(Class *object, void (Class::*method)(QueryResult_AutoPtr, ParamType1, ParamType2), ParamType1 param1, ParamType2 param2, const char *sql);
        template<class Class, typename ParamType1, typename ParamType2, typename ParamType3>
            bool AsyncQuery(Class *object, void (Class::*method)(QueryResult_AutoPtr, ParamType1, ParamType2, ParamType3), ParamType1 param1, ParamType2 param2, ParamType3 param3, const char *sql);
        // Query / static
        template<typename ParamType1>
            bool AsyncQuery(void (*method)(QueryResult_AutoPtr, ParamType1), ParamType1 param1, const char *sql);
        template<typename ParamType1, typename ParamType2>
            bool AsyncQuery(void (*method)(QueryResult_AutoPtr, ParamType1, ParamType2), ParamType1 param1, ParamType2 param2, const char *sql);
        template<typename ParamType1, typename ParamType2, typename ParamType3>
            bool AsyncQuery(void (*method)(QueryResult_AutoPtr, ParamType1, ParamType2, ParamType3), ParamType1 param1, ParamType2 param2, ParamType3 param3, const char *sql);
        // PQuery / member
        template<class Class>
            bool AsyncPQuery(Class *object, void (Class::*method)(QueryResult_AutoPtr), const char *format,...) ATTR_PRINTF(4,5);
        template<class Class, typename ParamType1>
            bool AsyncPQuery(Class *object, void (Class::*method)(QueryResult_AutoPtr, ParamType1), ParamType1 param1, const char *format,...) ATTR_PRINTF(5,6);
        template<class Class, typename ParamType1, typename ParamType2>
            bool AsyncPQuery(Class *object, void (Class::*method)(QueryResult_AutoPtr, ParamType1, ParamType2), ParamType1 param1, ParamType2 param2, const char *format,...) ATTR_PRINTF(6,7);
        template<class Class, typename ParamType1, typename ParamType2, typename ParamType3>
            bool AsyncPQuery(Class *object, void (Class::*method)(QueryResult_AutoPtr, ParamType1, ParamType2, ParamType3), ParamType1 param1, ParamType2 param2, ParamType3 param3, const char *format,...) ATTR_PRINTF(7,8);
        // PQuery / static
        template<typename ParamType1>
            bool AsyncPQuery(void (*method)(QueryResult_AutoPtr, ParamType1), ParamType1 param1, const char *format,...) ATTR_PRINTF(4,5);
        template<typename ParamType1, typename ParamType2>
            bool AsyncPQuery(void (*method)(QueryResult_AutoPtr, ParamType1, ParamType2), ParamType1 param1, ParamType2 param2, const char *format,...) ATTR_PRINTF(5,6);
        template<typename ParamType1, typename ParamType2, typename ParamType3>
            bool AsyncPQuery(void (*method)(QueryResult_AutoPtr, ParamType1, ParamType2, ParamType3), ParamType1 param1, ParamType2 param2, ParamType3 param3, const char *format,...) ATTR_PRINTF(6,7);
        template<class Class>
        // QueryHolder
            bool DelayQueryHolder(Class *object, void (Class::*method)(QueryResult_AutoPtr, SqlQueryHolder*), SqlQueryHolder *holder);
        template<class Class, typename ParamType1>
            bool DelayQueryHolder(Class *object, void (Class::*method)(QueryResult_AutoPtr, SqlQueryHolder*, ParamType1), SqlQueryHolder *holder, ParamType1 param1);

        bool Execute(const char *sql);
        bool PExecute(const char *format,...) ATTR_PRINTF(2,3);
        bool DirectExecute(const char* sql);
        bool DirectPExecute(const char *format,...) ATTR_PRINTF(2,3);

        // Writes SQL commands to a LOG file (see worldserver.conf "LogSQL")
        bool PExecuteLog(const char *format,...) ATTR_PRINTF(2,3);
        bool DirectPExecuteLog(const char *format,...) ATTR_PRINTF(2,3);

        bool BeginTransaction();
        bool CommitTransaction();
        bool RollbackTransaction();

        operator bool () const { return mMysql != NULL; }
        unsigned long escape_string(char *to, const char *from, unsigned long length);
        void escape_string(std::string& str);

        void ThreadStart();
        void ThreadEnd();

        // sets the result queue of the current thread, be careful what thread you call this from
        void SetResultQueue(SqlResultQueue * queue);

    private:
        bool m_logSQL;
        std::string m_logsDir;
        ACE_Thread_Mutex mMutex;        // For thread safe operations between core and mySQL server
        ACE_Thread_Mutex nMutex;        // For thread safe operations on m_transQueues

        ACE_Based::Thread * tranThread;

        MYSQL *mMysql;

        static size_t db_count;

        bool _TransactionCmd(const char *sql);
        bool _Query(const char *sql, MYSQL_RES **pResult, MYSQL_FIELD **pFields, uint64* pRowCount, uint32* pFieldCount);
};
#endif

