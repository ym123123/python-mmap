from ctypes import CDLL, c_void_p, c_int, c_char_p, create_string_buffer, byref

class my_void_p (c_void_p):
    pass 

class frame(object):
    RET_OK  =   0
    RET_AGAIN   =   1
    RET_ERR = -1  
    __lib = CDLL('./libframe.so')
    __lib.get_frame_data.restype = my_void_p
    __lib.init_frame_cursor.restype = my_void_p
    __lib.create_frame.restype = my_void_p 
    __lib.get_frame_header.restype = my_void_p 
    __lib.frame_data.restype = c_char_p
    def __init__(self):
        self.frame = None
        self.cur = frame.__lib.init_frame_cursor()
        
    @staticmethod
    def frame_init_pool(totle, align, fs_len):
        ret = frame.__lib.frame_init_pool(totle, align, fs_len)
        
        if ret == frame.RET_ERR:
            raise Exception('init frame error.')
    def create_frame(self, ip, port):
        self.frame = frame.__lib.create_frame(ip, port)
        if self.frame.value == None:
            raise Exception('create frame error.')
    
    def destroy_frame(self):
        return frame.__lib.destroy_frame(self.frame)
    
    def frame_push(self, data, index):
        ret = frame.__lib.frame_push(self.frame, data, len(data), index)
        
        if ret == frame.RET_ERR:
            raise Exception('frame push error.')
        
        return ret
    
    def get_frame_header(self, ip, port):
        self.frame = frame.__lib.get_frame_header(ip, port)

        if self.frame.value == None:
            return frame.RET_AGAIN
        return frame.RET_OK
        
    def put_frame_header(self):
        if self.frame.value != None:
            frame.__lib.put_frame_header(self.frame)
        return frame.RET_OK
    
    def frame_pop(self):
        cur = frame.__lib.get_frame_data(self.frame, self.cur)
        if cur.value == None:
            raise Exception('get frame error.')
        data_len = c_int(0)
        ds = frame.__lib.frame_data(cur, byref(data_len))
        if ds == None:
            return (frame.RET_AGAIN, '')
        ds = create_string_buffer(ds, data_len.value)
        self.cur = cur
        return (frame.RET_OK, ds)
        
        
