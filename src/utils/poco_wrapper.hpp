#ifndef POCO_WRAPPER_HPP
#define POCO_WRAPPER_HPP
/*
poco_wrapper.hpp
2014/12/21
psycommando@gmail.com
Description: A wrapper to avoid including POCO for doing some common file operations.
*/
#include <string>
#include <vector>

namespace utils
{
    // ---------------------------------------------------------------
    // File related stuff 
    // ---------------------------------------------------------------

    /************************************************************************
        ListDirContent_FilesAndDirs
            Write a list of all the files and folders in the directory 
            specified.

            -bFilenameOnly : If set to true, returns only the filename, 
                             not the complete path!
    ************************************************************************/
    std::vector<std::string> ListDirContent_FilesAndDirs( const std::string & dirpath, bool bFilenameOnly = false );

    /************************************************************************
        DoCreateDirectory
            Creates a new subdirectory at the path specified
    ************************************************************************/
    bool DoCreateDirectory( const std::string & path );

    /************************************************************************
        isFolder
            Returns true if the path specified is a directory and exists.
    ************************************************************************/
    bool isFolder( const std::string & inputpath );

    /************************************************************************
        isFile
            Returns true if the path specified is a file and exists.
    ************************************************************************/
    bool isFile( const std::string & inputpath );

    /************************************************************************
        pathExists
            Returns true if the path specified is valid and is either a file 
            or directory.
    ************************************************************************/
    bool pathExists( const std::string & inputpath );

    /************************************************************************
        getCWD
            Return the absolute path to the current working directory!
    ************************************************************************/
    std::string getCWD();

    /************************************************************************
        GetBaseNameOnly
            The basename is the the last dir in the path, or the name of the 
            file without extension.
    ************************************************************************/
    std::string GetBaseNameOnly( const std::string & path );

    /************************************************************************
        GetFileExtension
            Return the file extension only.
    ************************************************************************/
    std::string GetFileExtension( const std::string & path );

    /************************************************************************
        GetFilename
            Return the file name and extension only.
    ************************************************************************/
    std::string GetFilename( const std::string & path );
};

#endif