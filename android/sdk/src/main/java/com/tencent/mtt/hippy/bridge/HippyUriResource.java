package com.tencent.mtt.hippy.bridge;

import java.nio.ByteBuffer;

public class HippyUriResource {
  public enum RetCode {
    Success, Failed, DelegateError, UriError, SchemeError, SchemeNotRegister,
    PathNotMatch, PathError, ResourceNotFound, Timeout
  }

  RetCode code;
  ByteBuffer content;
}
