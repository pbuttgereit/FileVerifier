# FileVerifier

A simple tool to recursively create and check file hashes e.g. to supervise website source changes. Consider a web site that you want to supervise in order to find manipulations quickly (file system access is required).

First, a database need to be created with *Database\>New...* and then start to populate your initial repository e.g. with the function *Hash\>Directory*. This will recursively hash all files in all sub-directories and write the result including the path, file name, last change date and hash into a new database table (which is called A\<unix-epoch-in-seconds\>).
  
At a later point in time you may want to check the directory for manipulations. For this you open the database in the menu and select the table created in the step above within the combo-box.

Now choose *Verify\>Directory* and select the same directory again. As above a new table with the above mentioned content is created. Then, a loop over the new hashes checks for the exact same hashes in the previously created table. A report window opens with two text fields:

- For every new file hash that was also found in the initial table an entry is made into the text field "matches" with the hash and the two respective path and file names.

- For those files in the new table that do not have a matching hash (like new files and manipulated files) an according entry is made into the text field "misses". Each of the text fields can be exported to a tab-separated cvs text file (use txt as extension if you want to open it in Excel).

The same procedure can be done for a selection of files (using *Hash\>File(s)* and *Verify\>File(s)*, respectively). The hashing is performed using the standard QCryptographicHash class for SHA-512 to avoid deploying additional libraries.

Authors: P. Buttgereit, B. Martensen

License: Please refer to the LICENSE files next to the sources.
