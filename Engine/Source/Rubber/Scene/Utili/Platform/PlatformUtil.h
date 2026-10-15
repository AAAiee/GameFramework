#pragma once

#include <string>
namespace Rubber {

    /** Native Windows scene-path selection on the Application window's UI thread.
     * Requires a live Application. filter borrows a double-null-terminated Win32
     * display/pattern list for the duration of the modal call. Returned paths own
     * their UTF-16 storage. Both dialogs preserve the working directory and use a
     * 260-wchar_t buffer, including the terminating null (259 path code units).
     * Cancellation or common-dialog failure returns an empty string; these
     * outcomes are not distinguished. Extended-length path support is excluded.
     */
    class fileDialog {
    public:
        /** Selects an existing file in an existing directory; does not read it. */
        static std::wstring openFile(const wchar_t* filter);
        /** Allows a new file in an existing directory and confirms replacement.
         * Does not create/write the file; callers must skip writing on empty.
         * Declining replacement keeps the dialog open for another choice/cancel.
         */
        static std::wstring saveFile(const wchar_t* filter);
    };
}
