

## 1.0 alpha ##
  1. (20) STARTED: Drop UDP for IPC. Use TCP. Drop support for netid and gateway.
  1. (30) PENDING: Add support for read requests completed only by the coordinator.
  1. (40) PENDING: Improve consensus library by creating a usable authentication engine example
  1. (45) PENDING: make install
  1. (50) PENDING: API documentation.
  1. (55) PENDING: Testing and fixing.

## 1.0 beta ##
  1. (60) PENDING: More testing and fixing.

## 1.0 ##
  1. (70) PENDING: Update Wiki.

## Pool of pending features and changes ##
  1. (10) (AIO) Refactor aio::selector to deliver one event per socket at a time. ExecuteContext will contain socketId() which will return the id of the socket for which the events are addressed to.
  1. (20) Use (std/boost::system)::error\_code across solid.
  1. (30) (IPC) Add NoCompression message flag. This should be used for messages containing already compressed data. The ipc module will try to send without compression, most of the packets that the message is spread upon.
  1. (40) (IPC) Add support for adaptable compression:
    * A message is started with compression.
    * If certain continuous packets have compression ration below a certain threshold the compression is switched off for the rest of the message.
  1. Investigate the use of boost as back-end for solid::frame.
  1. Add support for Unix domain addresses.
  1. Add support to frame::Service to start the unique part of Object::uid() from a given value.

## Bugs ##

  1. Frame concept sample application doesn't stop when virtual machine is suspended then resumed.


## Done ##

  1. DONE:IPC gateway - relay IPC data through TCP for site-to-site communication.
  1. DONE: Move everything under solid and solid::frame namespaces
  1. DONE: Refactor solid::frame::Manager (ex. foundation::Manager)
  1. DONE: New binary protocol for client-server apps
  1. DONE: A distributed chat example (using the binary protocol) along with a stress client.
  1. DONE: Move the entire code under more liberal boost 1.0 license
  1. DONE: better response types - no more BAD/OK/NOK
  1. DONE: frame::shared::Store - asynchronous read/write lock.
  1. DONE: Refactor frame::file::Store to use frame::shared::Store
  1. DONE: Proper file rotation for debug
  1. DONE: Per service debug log level mask support and register all services on init
  1. DONE:IPC: Add support for authenticating the ipc sessions.
  1. DONE:IPC: Find a better way to disconnect after a failed authentication
  1. DONE:IPC: Add support for sending authentication commands while in authenticating state. Commands with AuthenticationFlag set.(NEEDS proper testing!!!!)
  1. DONE: IPC: Check that a failed authentication, raises the objects waiting for commands to be sent. There seems to be a bug!!
  1. DONE: Serialization: add support for specific objects: insertSpecific
  1. DONE: Try to optimize the memory usage for foundation::Object, related to the signal queue, by moving the queue onto the foundation::Service.
  1. DONE: Optimize specific memory cache by dropping usage of queues and using a simple linked list.
  1. DONE: Add support for compressing ipc buffers.
  1. DONE: Improve binary serialization performance and create better test applications
    * One should be able to register the same structure/class multiple times, every time being constructed on creation with a different constructor.
    * Also we need to be able to specify the position on which the map will be registered.
  1. DONE: Modify the ipc::Service to make use of the changes in serialization. We need to be able to specify the type id of a signal to be sent.
  1. DONE: Add support for pushUtf8(std::string&) to binary::serialization - the string will be serialized as null terminated data, i.e. no size.
  1. DONE: Add support for limits to binary::serialization. Add the following methods and changes to both serializer and deserializer:
    * constructor(const Limits ` * `pdefaultlimits);
    * resetLimits(); //reset all limits to default
    * pushStringLimit(uint32(256)) / pushStringLimit(), reset to default.
    * pushStreamLimit(uint64(200000000)) / pushStreamLimit(), reset to default.
    * pushContainerLimit(uint32()) - pushContainerLimit(), reset to default.
  1. DONE: Make system library compile on windows
  1. DONE: Find a fix for thread safe static member within static function.