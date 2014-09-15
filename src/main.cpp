#include <iostream>
#include "libevents.h"

using namespace std;

class A
{
public:
	A()
	{
		// First step is to register the events
		register_event("foo_event", make_event(foo));
		register_event("foo_event2", make_event(foo2));
	}

private:
	void foo(parameters params) { cout << "Executing foo" << endl; }
	void foo2(parameters params) { cout << "Executing foo2: " << params.at<int>(0) << endl; }
};

void foo3(parameters params)
{
	cout << "Executing foo3" << endl;
}

void foo4(parameters params)
{
	cout << "Executing foo4: " << params.at<int>(0) << endl;
}


int main()
{
	cout << "Example of libevents:" << endl;

	//Creating an instance of the object that will register the events
	A a;

	//Register the event for the simple function
	register_event("foo_event3", function_to_event(foo3));
	register_event("foo_event4", function_to_event(foo4));

	trigger_event("foo_event");
	trigger_event<int>("foo_event2", 5);
	trigger_event("foo_event3");
	trigger_event<int>("foo_event4", 5);

	return 0; 
}
