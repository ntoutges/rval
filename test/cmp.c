#include <stdio.h>
#include <assert.h>
#include "../rval.h"
#include "string.h"

rval_key_view_t create_view(const char* str);

int main() {
    rval_key_view_t view;

    // Normal use-case
    view = create_view("myKey");
    assert(rval_strcmp(view, "myKeys") < 0);
    assert(rval_strcmp(view, "myKez") < 0);
    assert(rval_strcmp(view, "myKey") == 0);
    assert(rval_strcmp(view, "myKex") > 0);
    assert(rval_strcmp(view, "myKe") > 0);
    assert(rval_strcmp(view, "") > 0);

    // Edge-case: Empty string
    view = create_view("");
    assert(rval_strcmp(view, "a") < 0);
    assert(rval_strcmp(view, "") == 0);

    return 0;
}

rval_key_view_t create_view(const char* str) {
    int length = strlen(str);

    return (rval_key_view_t) {
        .chars = str,
        .length = length
    };
}
