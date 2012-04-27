// 2008/05/01
#pragma once

class MayuDriver;
class KEYBOARD_INPUT_DATA;

int create_device(const char* device_name, const MayuDriver* driver);
void destroy_device();

 // キーイベントの受信通知
void push_keyevent(const KEYBOARD_INPUT_DATA* data, unsigned int count);

bool is_grabbed();
