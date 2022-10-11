This is the core of the BBComp software system. All functionality in the
original software that is concerned with client and server operation,
network communication, logging, etc. was removed. It remains the bare
minimum functionality required for evaluating objective functions used
in various competitions that took place between 2015 and 2019. Therefore
the interface differs slightly from the BBComp library:
 1. the history function was removed (with the logging functionality),
 2. the configure function was removed,
 3. the login function was removed,
 4. the library must be initialized by calling loadProblems(...),
 5. the current performance can be queried with performance().
Basic usage is demonstrated in the example program (example.c).
