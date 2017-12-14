
import tornado.gen 
from tornado.tcpserver import TCPServer
from frame import frame

class da(frame):
    def __init__(self, stream, address):
        frame.__init__(self)
        self.stream = stream 
        self.addr = address
        
    def __del__(self):
        self.stream.close()
        
    @tornado.gen.coroutine 
    def put_frame(self):
        while True:
            ret = self.destroy_frame()
            if ret == frame.RET_OK:
                return 
            yield tornado.gen.sleep(1)
        
    @tornado.gen.coroutine 
    def write_frame(self, ip, port):
        
        try:
            self.create_frame(ip, port)
            while True:
                data = yield self.stream.read_bytes(10240)
                ret = self.frame_push(data, 0)
                if ret == frame.RET_AGAIN:
                    print('da again again.')
                    self.frame_push(data, 0)
                
        except Exception as e:
            print(e)
        finally:
            yield self.put_frame()
     
    @tornado.gen.coroutine 
    def work(self):
        yield self.stream.read_bytes(10)
        yield self.write_frame(10, 10)
        
    

class da_listen(TCPServer):
    def __init__(self):
        TCPServer.__init__(self)
    
    def handle_stream(self, stream, address):
        loop = tornado.ioloop.IOLoop.instance()
        client = da(stream, address)
        future = client.work()
        
        loop.add_future(future, lambda f:f.result())
        


