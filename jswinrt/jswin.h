#ifndef JSWIN_H_
#define JSWIN_H_

#ifdef JSWIN_EXPORTS
#define JSWIN_API __declspec(dllexport)
#else
#define JSWIN_API __declspec(dllimport)
#endif

extern "C" 
{
    enum ErrorCodes
    {
        SUCCESS,
        GENERAL_ERROR
    };

    struct jswin_ctx;

    JSWIN_API jswin_ctx* jswin_init();
    JSWIN_API void jswin_shutdown(jswin_ctx* ctx);

    JSWIN_API int jswin_get_error_msg(jswin_ctx* ctx, char* message, int max_length);

    JSWIN_API int jswin_run_script(jswin_ctx* ctx, const char* module_id, int* exitCode);
}

#define JSWIN_SUCCEEDED(x) ((x) == SUCCESS)
#define JSWIN_FAILED(x) ((x) != SUCCESS)

#endif
