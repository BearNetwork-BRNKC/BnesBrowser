// Copyright (c) 2026 The BNES Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/bnes/bns_resolver.h"

#include <optional>
#include <string>
#include <string_view>

#include "base/check.h"
#include "base/containers/span.h"
#include "base/memory/ref_counted.h"
#include "brave/bnes/bns_security.h"
#include "brave/bnes/bns_constants.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "net/base/net_errors.h"
#include "net/http/http_response_headers.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace bnes {

namespace {

// Placeholder page: proves the bnes:// pipeline is live end-to-end until the
// real BNS registry (L6) replaces this resolution backend. Origin stays bnes://.
constexpr std::string_view kPlaceholderHtml = R"HTML(
<!doctype html><html><head><meta charset="utf-8">
<title>bnes:// resolved (stub)</title></head><body>
<h1>bnes://</h1>
<p>This is the BNES native-routing placeholder. The bnes:// URL reached the
browser-side loader. The real BNS registry resolution will replace this page.</p>
</body></html>)HTML";

}  // namespace

void ResolveBnesContent(
    const GURL& url,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client) {
  DCHECK(IsAllowedNavigationUrl(url));

  mojo::Remote<network::mojom::URLLoaderClient> client_remote(
      std::move(client));

  auto head = network::mojom::URLResponseHead::New();
  head->headers =
      base::MakeRefCounted<net::HttpResponseHeaders>("HTTP/1.1 200 OK");
  head->headers->AddHeader("Content-Type", "text/html; charset=utf-8");
  head->mime_type = "text/html";
  head->charset = "utf-8";

  const std::string body(kPlaceholderHtml);
  mojo::ScopedDataPipeProducerHandle producer;
  mojo::ScopedDataPipeConsumerHandle consumer;
  if (mojo::CreateDataPipe(body.size(), producer, consumer) == MOJO_RESULT_OK) {
    size_t bytes_written = 0;
    MojoResult write_result = producer->WriteData(
        base::as_bytes(base::span(body)), MOJO_WRITE_DATA_FLAG_ALL_OR_NONE,
        bytes_written);
    producer.reset();
    if (write_result == MOJO_RESULT_OK) {
      client_remote->OnReceiveResponse(std::move(head), std::move(consumer),
                                       std::nullopt);
      client_remote->OnComplete(
          network::URLLoaderCompletionStatus(net::OK));
      return;
    }
  }

  client_remote->OnComplete(
      network::URLLoaderCompletionStatus(net::ERR_INSUFFICIENT_RESOURCES));
}

bool ParseContenthash(std::string_view payload, std::string_view& out_cid) {
  // ENS IPFS contenthash encoding: 0xe3 || <varint prefix> || <CID bytes>.
  // We only accept the bare CID bytes after the 0xe3 tag and validate them
  // against the same rules as user-supplied CIDs.
  constexpr std::string_view kIpfsNamespaceTag = "\xe3";
  if (payload.size() < kIpfsNamespaceTag.size() ||
      !payload.starts_with(kIpfsNamespaceTag)) {
    return false;
  }

  std::string_view cid = payload.substr(kIpfsNamespaceTag.size());
  if (!IsValidCid(cid)) {
    return false;
  }

  out_cid = cid;
  return true;
}

bool IsAllowedGatewayUrl(const GURL& gateway_url) {
  return bnes::IsAllowedGatewayUrl(gateway_url, kDefaultIpfsGatewayHost);
}

bool ValidateAndBuildGateway(std::string_view cid,
                             std::string_view trusted_gateway_host,
                             GURL* out_gateway_url) {
  if (!IsValidCid(cid) || trusted_gateway_host.empty() || !out_gateway_url) {
    return false;
  }

  std::string gateway =
      std::string(url::kHttpsScheme).append("://")
          .append(trusted_gateway_host)
          .append(kIpfsPathPrefix)
          .append(cid);

  GURL url(gateway);
  if (!url.is_valid() || !IsAllowedGatewayUrl(url, trusted_gateway_host)) {
    return false;
  }

  *out_gateway_url = url;
  return true;
}

}  // namespace bnes
