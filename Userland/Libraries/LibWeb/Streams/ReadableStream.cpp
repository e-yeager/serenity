/*
 * Copyright (c) 2022, Linus Groh <linusg@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibJS/Runtime/PromiseCapability.h>
#include <LibWeb/Bindings/Intrinsics.h>
#include <LibWeb/Streams/AbstractOperations.h>
#include <LibWeb/Streams/ReadableStream.h>
#include <LibWeb/Streams/ReadableStreamDefaultController.h>
#include <LibWeb/Streams/ReadableStreamDefaultReader.h>
#include <LibWeb/Streams/UnderlyingSource.h>
#include <LibWeb/WebIDL/ExceptionOr.h>

namespace Web::Streams {

// https://streams.spec.whatwg.org/#rs-constructor
WebIDL::ExceptionOr<JS::NonnullGCPtr<ReadableStream>> ReadableStream::construct_impl(JS::Realm& realm, Optional<JS::Handle<JS::Object>> const& underlying_source_object)
{
    auto& vm = realm.vm();

    auto readable_stream = MUST_OR_THROW_OOM(realm.heap().allocate<ReadableStream>(realm, realm));

    // 1. If underlyingSource is missing, set it to null.
    auto underlying_source = underlying_source_object.has_value() ? JS::Value(underlying_source_object.value().ptr()) : JS::js_null();

    // 2. Let underlyingSourceDict be underlyingSource, converted to an IDL value of type UnderlyingSource.
    auto underlying_source_dict = TRY(UnderlyingSource::from_value(vm, underlying_source));

    // 3. Perform ! InitializeReadableStream(this).

    // 4. If underlyingSourceDict["type"] is "bytes":
    if (underlying_source_dict.type.has_value() && underlying_source_dict.type.value() == ReadableStreamType::Bytes) {
        // FIXME:
        // 1. If strategy["size"] exists, throw a RangeError exception.
        // 2. Let highWaterMark be ? ExtractHighWaterMark(strategy, 0).
        // 3. Perform ? SetUpReadableByteStreamControllerFromUnderlyingSource(this, underlyingSource, underlyingSourceDict, highWaterMark).
        TODO();
    }
    // 5. Otherwise,
    else {
        // 1. Assert: underlyingSourceDict["type"] does not exist.
        VERIFY(!underlying_source_dict.type.has_value());

        // FIXME: 2. Let sizeAlgorithm be ! ExtractSizeAlgorithm(strategy).
        SizeAlgorithm size_algorithm = [](auto const&) { return JS::normal_completion(JS::Value(1)); };

        // FIXME: 3. Let highWaterMark be ? ExtractHighWaterMark(strategy, 1).
        auto high_water_mark = 1.0;

        // 4. Perform ? SetUpReadableStreamDefaultControllerFromUnderlyingSource(this, underlyingSource, underlyingSourceDict, highWaterMark, sizeAlgorithm).
        TRY(set_up_readable_stream_default_controller_from_underlying_source(*readable_stream, underlying_source, underlying_source_dict, high_water_mark, move(size_algorithm)));
    }

    return readable_stream;
}

ReadableStream::ReadableStream(JS::Realm& realm)
    : PlatformObject(realm)
{
}

ReadableStream::~ReadableStream() = default;

// https://streams.spec.whatwg.org/#rs-locked
bool ReadableStream::locked()
{
    // 1. Return ! IsReadableStreamLocked(this).
    return is_readable_stream_locked(*this);
}

// https://streams.spec.whatwg.org/#rs-cancel
WebIDL::ExceptionOr<JS::GCPtr<JS::Object>> ReadableStream::cancel(JS::Value reason)
{
    auto& realm = this->realm();

    // 1. If ! IsReadableStreamLocked(this) is true, return a promise rejected with a TypeError exception.
    if (is_readable_stream_locked(*this)) {
        auto exception = MUST_OR_THROW_OOM(JS::TypeError::create(realm, "Cannot cancel a locked stream"sv));
        return WebIDL::create_rejected_promise(realm, JS::Value { exception })->promise();
    }

    // 2. Return ! ReadableStreamCancel(this, reason).
    return TRY(readable_stream_cancel(*this, reason))->promise();
}

// https://streams.spec.whatwg.org/#rs-get-reader
WebIDL::ExceptionOr<ReadableStreamReader> ReadableStream::get_reader()
{
    // FIXME:
    // 1. If options["mode"] does not exist, return ? AcquireReadableStreamDefaultReader(this).
    // 2. Assert: options["mode"] is "byob".
    // 3. Return ? AcquireReadableStreamBYOBReader(this).

    return TRY(acquire_readable_stream_default_reader(*this));
}

JS::ThrowCompletionOr<void> ReadableStream::initialize(JS::Realm& realm)
{
    MUST_OR_THROW_OOM(Base::initialize(realm));
    set_prototype(&Bindings::ensure_web_prototype<Bindings::ReadableStreamPrototype>(realm, "ReadableStream"));

    return {};
}

void ReadableStream::visit_edges(Cell::Visitor& visitor)
{
    Base::visit_edges(visitor);
    visitor.visit(m_controller);
    visitor.visit(m_stored_error);
    visitor.visit(m_reader);
}

// https://streams.spec.whatwg.org/#readablestream-locked
bool ReadableStream::is_readable() const
{
    // A ReadableStream stream is readable if stream.[[state]] is "readable".
    return m_state == State::Readable;
}

// https://streams.spec.whatwg.org/#readablestream-closed
bool ReadableStream::is_closed() const
{
    // A ReadableStream stream is closed if stream.[[state]] is "closed".
    return m_state == State::Closed;
}

// https://streams.spec.whatwg.org/#readablestream-errored
bool ReadableStream::is_errored() const
{
    // A ReadableStream stream is errored if stream.[[state]] is "errored".
    return m_state == State::Errored;
}
// https://streams.spec.whatwg.org/#readablestream-locked
bool ReadableStream::is_locked() const
{
    // A ReadableStream stream is locked if ! IsReadableStreamLocked(stream) returns true.
    return is_readable_stream_locked(*this);
}

// https://streams.spec.whatwg.org/#is-readable-stream-disturbed
bool ReadableStream::is_disturbed() const
{
    // A ReadableStream stream is disturbed if stream.[[disturbed]] is true.
    return m_disturbed;
}

}
