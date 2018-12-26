#pragma once

void* dlsym_weak(void* handle, const char* symbol);
void dlsym_replace(void* handle, const char* symbol, void* with);