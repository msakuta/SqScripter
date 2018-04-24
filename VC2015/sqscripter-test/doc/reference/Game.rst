Game
====

The singleton game object.
It represents the whole state of the game.

All the fields are static; you can call them like ``Game.time`` at any point in your script.

.. js:function:: Game.print(str)

   :param string str: string to print. Needs to be a string type (other types cause an error).

   Wren only. 
   Prints ``str`` to the console.

   Because Wren does not support bare functions, we need to put the print function into some class's member.
   Game class seemed the most appropriate, so here it is.

   Also note that unlike Squirrel's ``print`` function, this function requires the argument to be an acutal string.
   If you want to print simple numbers, use formatting like::

      Game.print("%(Game.time)")

   In Squirrel, you don't need ``Game.`` prefix.

.. js:attribute:: Game.time

   Returns global tick count. Read only.

.. js:attribute:: Game.creeps

   Returns a list of all :doc:`Creeps <Creep>` in the game.
   In Squirrel, the returned object is an array.
   In Wren, the returned object is a list (array equivalent).
   You can access individual Creeps by using suffix, e.g. ``Game.creeps[0]``.
   Note that Squirrel and Wren has different syntax for retrieving number of elements in an array (or a list).
   Squirrel uses ``len()`` method, while Wren uses ``count`` attribute.

.. js:attribute:: Game.spawns

   Returns a list of all :doc:`Spawns <Spawn>` in the game.

.. js:attribute:: Game.mines

   Returns a list of all :doc:`Mines <Mine>` in the game.

.. js:attribute:: Game.races

   Returns a list of all :doc:`Races <Race>` in the game.
