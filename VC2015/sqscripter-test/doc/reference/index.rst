.. Sqreeps API Documentation documentation master file, created by
   sphinx-quickstart on Mon Apr 16 00:45:17 2018.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

API Reference
=============

Sqreeps has two embedded scripting languages, i.e. `Squirrel <http://www.squirrel-lang.org/>`_ and `Wren <http://wren.io/>`_.
You can use either one, but I would not recommend using both mixed, since
there is no way to communicate data between these languages.

In this document, classes and its member fields and methods are described.
The interface is the same among Squirrel and Wren, so the documentation has
no distinction between them.

.. toctree::
   :maxdepth: 2

   Game.rst
   Race
   RoomPosition.rst
   Creep
   Spawn
   Mine

