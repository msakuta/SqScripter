Creep
=====

A moving unit on a room.
A Creep is created from a :doc:`Spawn`.
A Creep has maximum time to live. Any Creep will die after certain amount of time.
You can check the remaining live of a Creep by retrieving ``ttl`` field.

.. js:attribute:: Creep.alive

   Returns ``true`` if the object is alive. You should check if it's alive before accessing other fields or methods.

.. js:attribute:: Creep.pos

   Returns position of this Creep in a format of :doc:`RoomPosition`. Read only.

.. js:attribute:: Creep.id

   Returns a number that uniquely identifies all RoomObjects. Read only.

.. js:attribute:: Creep.owner

   A number indicating the ID of the owner.

.. js:attribute:: Creep.ttl

   Time to live, which means how many ticks this Creep will live before expiring. Read only.

.. js:attribute:: Creep.resource

   How much resource this Creep is carrying. Read only.

.. js:attribute:: Creep.memory

   A special field that can contain arbitrary data in the scripts.
   You can set any type of data into this field and read it back later at any time.
   The stored object is not freed even a tick is elapsed.

   Example in Squirrel:

   .. code-block:: JavaScript

       Game.creeps[0].memory = "Hello"
       
       print(Game.creeps[0].memory) // prints "Hello"

   Please note that it's not necessarily true for RoomObjects.
   Those objects who has ``alive`` property can become dead between ticks.
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

   Search the path to the destination and stores it into internal buffer of this Creep.
   Returns ``true`` if the path was found, ``false`` otherwise.

.. js:function:: Creep.followPath()

   Move this Creep so that it follows the last searched path with :js:func:`Creep.findPath`.


