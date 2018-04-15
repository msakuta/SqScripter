Mine
====

An environmental stationary structure that can have resources.
Mines can randomly pop up in the room.
You can gather resources in a Mine with a :doc:`Creep`.
If the resources runs out, the structure disappears.

Unlike Spawns, no one controls a mine, so it doesn't have an owner.

.. js:attribute:: Mine.alive

   Returns ``true`` if the object is alive. You should check if it's alive before accessing other fields or methods.

.. js:attribute:: Mine.pos

   Returns position of this Mine in a format of :doc:`RoomPosition`. Read only.

.. js:attribute:: Mine.id

   Returns a number that uniquely identifies all RoomObjects. Read only.

.. js:attribute:: Mine.resource

   How much resource this Mine has. Read only.
