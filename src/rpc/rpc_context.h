// Copyright (c) 2013, Cloudera, inc.
#ifndef KUDU_RPC_RPC_CONTEXT_H
#define KUDU_RPC_RPC_CONTEXT_H

#include <string>

#include "gutil/gscoped_ptr.h"
#include "rpc/service_if.h"
#include "util/status.h"

namespace google {
namespace protobuf {
class Message;
class MessageLite;
} // namespace protobuf
} // namespace google

namespace kudu {

class Sockaddr;
class Trace;

namespace rpc {

class UserCredentials;
class InboundCall;

// The context provided to a generated ServiceIf. This provides
// methods to respond to the RPC. In the future, this will also
// include methods to access information about the caller: e.g
// authentication info, tracing info, and cancellation status.
//
// This is the server-side analogue to the RpcController class.
class RpcContext {
 public:
  // Create an RpcContext. This is called only from generated code
  // and is not a public API.
  RpcContext(InboundCall *call,
             const google::protobuf::Message *request_pb,
             google::protobuf::Message *response_pb,
             RpcMethodMetrics metrics);

  ~RpcContext();

  // Return the trace buffer for this call.
  Trace* trace();

  // Send a response to the call. The service may call this method
  // before or after returning from the original handler method,
  // and it may call this method from a different thread.
  //
  // The response should be prepared already in the response PB pointer
  // which was passed to the handler method.
  //
  // After this method returns, this RpcContext object is destroyed. The request
  // and response protobufs are also destroyed.
  void RespondSuccess();

  // Respond with an error to the client. This sends back an error with the code
  // ERROR_APPLICATION. Because there is no more specific error code passed back
  // to the client, most applications should create a custom error PB extension
  // and use RespondApplicationError(...) below. This method should only be used
  // for unexpected errors where the server doesn't expect the client to do any
  // more advanced handling.
  //
  // After this method returns, this RpcContext object is destroyed. The request
  // and response protobufs are also destroyed.
  void RespondFailure(const Status &status);

  // Respond with an application-level error. This causes the caller to get a
  // RemoteError status with the provided string message. Additionally, a
  // service-specific error extension is passed back to the client. The
  // extension must be registered with the ErrorStatusPB protobuf. For
  // example:
  //
  //   message MyServiceError {
  //     extend kudu.rpc.ErrorStatusPB {
  //       optional MyServiceError my_service_error_ext = 101;
  //     }
  //     // Add any extra fields or status codes you want to pass back to
  //     // the client here.
  //     required string extra_error_data = 1;
  //   }
  //
  // NOTE: the numeric '101' above must be an integer greater than 101
  // and must be unique across your code base.
  //
  // Given the above definition in your service protobuf file, you would
  // use this method like:
  //
  //   MyServiceError err;
  //   err.set_extra_error_data("foo bar");
  //   ctx->RespondApplicationError(MyServiceError::my_service_error_ext.number(),
  //                                "Some error occurred", err);
  //
  // The client side may then retreieve the error by calling:
  //   const MyServiceError& err_details =
  //     controller->error_response()->GetExtension(MyServiceError::my_service_error_ext);
  //
  // After this method returns, this RpcContext object is destroyed. The request
  // and response protobufs are also destroyed.
  void RespondApplicationError(int error_ext_id, const std::string& message,
                               const google::protobuf::MessageLite& app_error_pb);

  // Return the credentials of the remote user who made this call.
  const UserCredentials& user_credentials() const;

  // Return the remote IP address and port which sent the current RPC call.
  const Sockaddr& remote_address() const;

  // A string identifying the requestor -- both the user info and the IP address.
  // Suitable for use in log messages.
  std::string requestor_string() const;

  const google::protobuf::Message *request_pb() const { return request_pb_.get(); }
  google::protobuf::Message *response_pb() const { return response_pb_.get(); }

 private:
  InboundCall *call_;
  const gscoped_ptr<const google::protobuf::Message> request_pb_;
  const gscoped_ptr<google::protobuf::Message> response_pb_;
  RpcMethodMetrics metrics_;
};

} // namespace rpc
} // namespace kudu
#endif
