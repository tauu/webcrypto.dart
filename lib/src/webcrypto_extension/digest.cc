#include <assert.h>
#include <openssl/digest.h>
#include <openssl/err.h>

#include "digest.h"
#include "utils.h"

Dart_Handle HandleError(Dart_Handle handle) {
  if (Dart_IsError(handle)) {
    Dart_PropagateError(handle);
  }
  return handle;
}

const EVP_MD* hashIdentifierToAlgorithm(int hashIdentifier) {
  switch (hashIdentifier) {
    case 0:
      return EVP_sha1();
    case 1:
      return EVP_sha256();
    case 2:
      return EVP_sha384();
    case 3:
      return EVP_sha512();
    default:
      assert(false);
      return nullptr;
  }
}

/// Signature: (int hashIdentifier) -> int | String
void digest_create(Dart_NativeArguments args) {
  ARGUMENT_COUNT_OR_RETURN(1);
  DEFINE_INT_ARG_OR_RETURN(0, hashIdentifier);

  // Get the algorithm
  auto algorithm = hashIdentifierToAlgorithm(hashIdentifier);
  if (algorithm == nullptr) {
    RETURN_API_ERROR("invalid hash identifier");
  }

  // Allocate a context
  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if (ctx == nullptr) {
    SET_RETURN_VALUE_TO_BORINGSSL_ERROR_STRING();
    return;
  }

  // Initialize the context
  if (EVP_DigestInit(ctx, algorithm) != 1) {
    SET_RETURN_VALUE_TO_BORINGSSL_ERROR_STRING();
    // Release the context
    EVP_MD_CTX_free(ctx);
    return;
  }

  // Return pointer to the context
  Dart_SetIntegerReturnValue(args, (int64_t)ctx);
}

/// Signature: (int ctx, Uint8List data) -> Null | String
void digest_write(Dart_NativeArguments args) {
  ARGUMENT_COUNT_OR_RETURN(2);
  DEFINE_INT_ARG_OR_RETURN(0, _ctx);
  EVP_MD_CTX* ctx = (EVP_MD_CTX*)_ctx;

  DEFINE_UINT8LIST_ARG_OR_RETURN(1, dataHandle);
  ACCESS_UINT8LIST_OR_RETURN(dataHandle, data, length);

  if (EVP_DigestUpdate(ctx, (void*)data, length) != 1) {
    SET_RETURN_VALUE_TO_BORINGSSL_ERROR_STRING();
    return;
  }

  Dart_SetReturnValue(args, Dart_Null());
}

/// Signature: (int ctx) -> Uint8List | String
void digest_result(Dart_NativeArguments args) {
  ARGUMENT_COUNT_OR_RETURN(1);
  DEFINE_INT_ARG_OR_RETURN(0, _ctx);
  EVP_MD_CTX* ctx = (EVP_MD_CTX*)_ctx;

  // Get the digest size
  size_t result_size = EVP_MD_CTX_size(ctx);

  // Allocate an Uint8List for the result
  auto dataHandle = Dart_NewTypedData(Dart_TypedData_kUint8, result_size);
  NOT_ERROR_OR_RETURN(dataHandle);
  ACCESS_UINT8LIST_OR_RETURN(dataHandle, data, length);

  // Extract the final result
  if (EVP_DigestFinal(ctx, data, nullptr) != 1) {
    SET_RETURN_VALUE_TO_BORINGSSL_ERROR_STRING();
    return;
  }

  Dart_SetReturnValue(args, dataHandle);
}

/// Signature: (int ctx) -> Null | String
void digest_destroy(Dart_NativeArguments args) {
  ARGUMENT_COUNT_OR_RETURN(1);
  DEFINE_INT_ARG_OR_RETURN(0, _ctx);
  EVP_MD_CTX* ctx = (EVP_MD_CTX*)_ctx;

  EVP_MD_CTX_free(ctx);

  Dart_SetReturnValue(args, Dart_Null());
}