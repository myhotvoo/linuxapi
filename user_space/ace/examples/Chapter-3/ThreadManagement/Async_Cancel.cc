// $Id: Async_Cancel.cpp 83251 2008-10-15 10:55:13Z vzykov $

#include <ace/OS_NS_unistd.h>
#include <ace/Task.h>
#include <ace/Log_Msg.h>

/*
 * EXTRA_CMDS=pkg-config --cflags --libs ACE
 */

class CanceledTask : public ACE_Task<ACE_MT_SYNCH> {
public:
	virtual int svc(void) {
		ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%t) Starting thread\n")));

		if (this->set_cancel_mode() < 0) {
			return(-1);
		}

		while (1) {
			// Put this thread in a compute loop.. no
			// cancellation points are available.
		}
#if defined (__HP_aCC)
		// This is only to workaround a warning on HP-UX compiler.
		return(0);
#endif /* __HP_aCC */
	}


	int set_cancel_mode(void) {
		cancel_state new_state;

		// Set the cancel state to asynchronous and enabled.
		new_state.cancelstate = PTHREAD_CANCEL_ENABLE;
		new_state.canceltype = PTHREAD_CANCEL_ASYNCHRONOUS;
		if (ACE_Thread::setcancelstate(new_state, 0) == -1) {
			ACE_ERROR_RETURN((LM_ERROR,
			                  ACE_TEXT("%p\n"),
			                  ACE_TEXT("cancelstate")), -1);
		}
		return(0);
	}
};
// Listing 1
// Listing 2 code/ch13
int ACE_TMAIN(int, ACE_TCHAR *[]) {
	CanceledTask task;

	task.activate();
	ACE_OS::sleep(1);
	ACE_Thread_Manager::instance()->cancel_task(&task, 1);
	task.wait();

	return(0);
}
