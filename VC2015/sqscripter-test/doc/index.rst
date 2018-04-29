.. Sqreeps API Documentation documentation master file, created by
   sphinx-quickstart on Mon Apr 16 00:45:17 2018.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to Sqreeps API Documentation!
=====================================

Sqreeps is a game about scripting.
You write a source code in a scripting language and control the game.
In this game, scripting API is the most important thing to play.


Execution Model
===============

Sqreeps has a data model similar to Screeps.
Simulation runs in real time and the only way the player can interact with the simulation is
via scripting.

You can retrieve current status of the game and place commands to the units he/she is controlling.


Weak References
===============

Sqreeps uses a concept called the weak reference to manage lifetime of in-game objects.
It becomes automatically invalid when the actual object is destructed.

It is important to understand the concept because the game itself can delete objects at any point of the game simulation,
but the scripts has no mechanism to reliably receive the event and update references.

In particular, script VMs can have references to in-game objects in the memory, which can span among ticks,
but the objects can be destroyed in these ticks in-between.
In the world of garbage-collected scripting languages, these old references are usually kept until the last reference to them
are erased, but this approach doesn't work quite well.
How can you tell a given object is already gone, if the object is still allocated in the memory even after removed from the
game object list?

In the simulation with dynamic object creation and deletion, things gone are gone.
There is no point waiting deletion until the last reference to them are erased (unless you want post-mortem information).
So we use weak pointers to achieve this.

Every dynamic object in Sqreeps has an attribute named ``alive``.
The attribute is initialized with ``true`` on object creation.
When the object is destroyed, the attribute becomes ``false`` and every other attribute and method becomes invalid
(calling them causes a run-time error).
The real object is already destroyed, only a reference to it remains.

So, you should check ``alive`` attribute before performing any operation on an object, if you got the reference from previous ticks.
There are three ways to get the reference from previous ticks.

* Global variables.
* Object-associated memory, such as :ref:`Creep.memory`.
* local variables in coroutines (or generators or fibers).



Screeps' way
------------

Screeps, an inspiring game runs on the browser with similar concept, dealed with this issue by serializing everything,
every tick, to strings.
However, this approach has significant drawback on performance.
Translation of objects to strings and vice-versa is not an easy task.
Of course, Screeps has another good reason to do this, which is to save the game state to the server, but you need to do that
every tick.
Also, serialization disbales the use of coroutines, which makes programming the behavior of Creeps less fun.


Internals
---------

Internally, a concept called observer scheme is used to achieve weak references.
Weak references are instances of Observer abstract class.
Observers can have references to Observable class objects, which has a list of Observers.
When a Observable is destroyed, all relevant Observers are notified via the destructor.
Observer instances clears the pointers accordingly.

This way, there will be no dangling pointers, which could cause access violation.


API Reference
=============

.. toctree::
   :maxdepth: 1

   reference/index.rst



Indices and tables
==================

* :ref:`genindex`

.. Modules and Search page does not work
   * :ref:`modindex`
   * :ref:`search`
