Spawn
=====

A stationary structure that can produce :doc:`Creeps <Creep>` from resources.

.. js:attribute:: Spawn.alive

   Returns ``true`` if the object is alive. You should check if it's alive before accessing other fields or methods.

.. js:attribute:: Spawn.pos

   Returns position of this Spawn in a format of :doc:`RoomPosition`. Read only.

.. js:attribute:: Spawn.id

   Returns a number that uniquely identifies all RoomObjects. Read only.

.. js:attribute:: Spawn.owner

   A number indicating the ID of the owner.

.. js:attribute:: Spawn.resource

   How much resource this Spawn has. Read only.

.. js:function:: Spawn.createCreep()

   Creates a Creep and place it in an adjacent tile.
   You cannot choose which direction newly created Creep will go.
   This function will fail if there is not enough room to create a new Creep.
