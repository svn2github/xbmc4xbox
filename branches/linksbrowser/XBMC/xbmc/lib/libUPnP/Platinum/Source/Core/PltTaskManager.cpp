/*****************************************************************
|
|   Platinum - Task Manager
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptSystem.h"
#include "PltTaskManager.h"
#include "PltThreadTask.h"

/*----------------------------------------------------------------------
|   PLT_TaskManager::PLT_TaskManager
+---------------------------------------------------------------------*/
PLT_TaskManager::PLT_TaskManager(NPT_Cardinal max_items /* = 0 */) :
    m_Queue(max_items?new NPT_Queue<NPT_Integer>(max_items):NULL)
{
}

/*----------------------------------------------------------------------
|   PLT_TaskManager::~PLT_TaskManager
+---------------------------------------------------------------------*/
PLT_TaskManager::~PLT_TaskManager()
{    
    StopAllTasks();
}

/*----------------------------------------------------------------------
|   PLT_TaskManager::StartTask
+---------------------------------------------------------------------*/
NPT_Result 
PLT_TaskManager::StartTask(PLT_ThreadTask*   task, 
                           NPT_TimeInterval* delay /* = NULL*/,
                           bool              auto_destroy /* = true */)
{
    return task->Start(this, delay, auto_destroy);
}

/*----------------------------------------------------------------------
|   PLT_TaskManager::StopTask
+---------------------------------------------------------------------*/
NPT_Result
PLT_TaskManager::StopTask(PLT_ThreadTask* task)
{
    {
        NPT_AutoLock lock(m_TasksLock);
        // if task is not found, then it might have been autodestroyed already
        NPT_CHECK(m_Tasks.Remove(task));
    }

    task->Stop();

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_TaskManager::StopAllTasks
+---------------------------------------------------------------------*/
NPT_Result
PLT_TaskManager::StopAllTasks()
{
    // unblock the queue if any
    if (m_Queue) {
        NPT_Queue<NPT_Integer>* queue = m_Queue;
        m_Queue = NULL;
        delete queue;
    }  

    // stop all tasks first
    {      
        NPT_AutoLock lock(m_TasksLock);
        NPT_List<PLT_ThreadTask*>::Iterator task = m_Tasks.GetFirstItem();
        while (task) {
            (*task)->Stop();
            ++task;
        }
    }

    // then wait for list to become empty
    // as tasks remove themselves from the list
    while (1) {
        {
            NPT_AutoLock lock(m_TasksLock);
            if (m_Tasks.GetItemCount() == 0)
                return NPT_SUCCESS;
        }

        NPT_System::Sleep(NPT_TimeInterval(0, 100));
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_TaskManager::AddTask
+---------------------------------------------------------------------*/
NPT_Result 
PLT_TaskManager::AddTask(PLT_ThreadTask* task) 
{
    if (m_Queue) {
        NPT_CHECK(m_Queue->Push(new NPT_Integer));
    }

    {
        NPT_AutoLock lock(m_TasksLock);
        return m_Tasks.Add(task);
    }
}

/*----------------------------------------------------------------------
|   PLT_TaskManager::RemoveTask
+---------------------------------------------------------------------*/
// called by a ThreadTask::Run when done
NPT_Result
PLT_TaskManager::RemoveTask(PLT_ThreadTask* task)
{
    if (m_Queue) {
        NPT_Integer* val = NULL;
        if (NPT_SUCCEEDED(m_Queue->Pop(val)))
            delete val;
    }

    {
        NPT_AutoLock lock(m_TasksLock);
        m_Tasks.Remove(task);
    }
    
    if (task->m_AutoDestroy) delete task;

    return NPT_SUCCESS;
}
