Creep
=====

A moving unit on a room.
A Creep is created from a :doc:`Spawn`.
A Creep has maximum time to live. Any Creep will die after certain amount of time.
You can check the remaining time to live of a Creep by retrieving ``ttl`` field.

.. js:attribute:: Creep.alive

   Returns ``true`` if the object is alive. You should check if it's alive before accessing other fields or methods.

.. js:attribute:: Creep.pos

   Returns position of this Creep in a format of :doc:`RoomPosition`. Read only.

.. js:attribute:: Creep.id

   Returns a number that uniquely identifies all RoomObjects. Read only.

.. js:attribute:: Creep.owner

   A number indicating the ID of the owner.
   This is an index to :js:attr:`Game.races` array.

.. js:attribute:: Creep.ttl

   Time to live, which means how many ticks this Creep will live before expiring. Read only.

.. js:attribute:: Creep.resource

   How much resource this Creep is carrying. Read only.

.. js:attribute:: Creep.moveParts

   The number of move parts this Creep has. Read only.

.. js:attribute:: Creep.memory

   A special field that can contain arbitrary data in the scripts.
   You can set any type of data into this field and read it back later at any time.
   The stored object is not freed even after a tick is elapsed.

   Example in Squirrel:

   .. code-block:: JavaScript

       Game.creeps[0].memory = "Hello"
       
       print(Game.creeps[0].memory) // prints "Hello"

   Please note that, as described in :doc:`Weak Reference <../index>`, references to other RoomObjects in this memory can become invalid
   when a tick elapses.
   You should check if the object is alive before accessing its contents like below.

   .. code-block:: JavaScript

      Game.creeps[0].memory = Game.spawns[0]
      
      function main(){
         if(Game.creeps[0].memory.alive)
         	print(Game.creeps[0].memory.id)
      }

.. js:function:: Creep.move(direction)

   :param direction: A direction ID to move.

   Order this Creep to move to designated direction.
   The number and direction in deltas are defined as follows.::

      { 0,-1 }, // TOP = 1,
      { 1,-1 }, // TOP_RIGHT = 2,
      { 1,0 }, // RIGHT = 3,
      { 1,1 }, // BOTTOM_RIGHT = 4,
      { 0,1 }, // BOTTOM = 5,
      { -1,1 }, // BOTTOM_LEFT = 6,
      { -1,0 }, // LEFT = 7,
      { -1,-1 }, // TOP_LEFT = 8,

   However, it is generally very difficult to determine which direction you shold step to in order to approach your final destination,
   because there can be obstacles in complex forms, possibly changing over time.
   For this purpose, C++-natively optimized :js:func:`Creep.findPath` and :js:func:`Creep.followPath` can be used.

.. js:function:: Creep.harvest(direction)

   :param direction: Ignored.

   Order this Creep to harvest an adjacent Mine.

.. js:function:: Creep.store(direction)

   :param direction: Ignored.

   Order this Creep to store havested resources into adjacent Spawn.

.. js:function:: Creep.attack(direction)

   :param direction: Ignored.

   Order this Creep to attack an enemy Creep at an adjacent tile.
   Attacking costs resources, so the Creep needs to have at least some resources to perfom attacking.

.. js:function:: Creep.findPath(to)

   :param RoomPosition to: the destination to search path to.

   Search the path to the destination using A* path finding algorithm and stores it into internal buffer of this Creep.
   Returns ``true`` if the path was found, ``false`` otherwise.
   You can subsequently call :js:func:`Creep.followPath` to move one step towards the destination.

   Note that path finding can be expensive in terms of CPU load, so you may want to call it only once in a while.
   However, you will need to call it periodically because the situation (location of other RoomObjects) can change over time
   and the Creep may need to adapt to the new situation.

.. js:function:: Creep.followPath()

   Move this Creep so that it follows the last searched path with :js:func:`Creep.findPath`.
   Returns ``true`` if the next move is not blocked.

   If ``true`` is returned, it also pops the last element of internal path array, which means calling this function periodically
   will eventually bring this Creep to the destination, given that the path is not blocked by another moving object.


