# FileVerifier
Tool to recursively create and check file hashes e.g. to supervise website sources

Consider a web site that You want to supervise in order to find manipulations quickly.

First, You need to create a SQLite database: File>Database
Then, open e.g. Your htdocs directory from File>Open>Directory

This will recursively hash all files in all sub-directories and write the result including the path, file name, last change date and hash into a new database table (which is called A<unix-epoch-in-seconds>).
  
At a later point in time You may want to check the directory for manipulations. For this You open the SQLite database created in the steps above and select the table created in the step above by selecting the table in the combobox.
Now choose Verify>Directory and choose htdocs again. As above a new table with the above mentioned content is created. Then, a loop over the new hashes checks for the exact same hashes in the previously created table. 
A report window opens with two text fields: For every new file hash that was also found in the initial table an entry is made into the text field "matches" with the hash and the two respective path and file names.
For those files in the new table that do not have a matching hash (like new files and manipulated files) an according entry is made into the text field "misses". Each of the text fields can be exported to a tab-separated cvs text file (use txt as extension if You want to open it in Excel).

The same procedure can be done for a selection of files (using File>Open>Files and Verify>Files, respectively).

The hashing is performed using the standard QCryptographicHash class to avoid deploying additional libraries. The hash type SHA512 is hard coded.

