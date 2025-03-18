/*
 * Copyright 2018 The gRPC Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package ru.yandex.ci.common.grpc;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;

import javax.annotation.Nonnull;

import com.google.common.base.Stopwatch;
import io.grpc.ManagedChannel;
import io.grpc.Server;
import org.junit.jupiter.api.extension.AfterEachCallback;
import org.junit.jupiter.api.extension.ExtensionContext;

import static com.google.common.base.Preconditions.checkNotNull;

public class GrpcCleanupExtension implements AfterEachCallback {

    private final List<Resource> resources = new ArrayList<>();
    private final long timeoutNanos = TimeUnit.SECONDS.toNanos(10L);
    private final Stopwatch stopwatch = Stopwatch.createUnstarted();

    public <T extends ManagedChannel> T register(@Nonnull T channel) {
        checkNotNull(channel, "channel");
        register(new ManagedChannelResource(channel));
        return channel;
    }

    public <T extends Server> T register(@Nonnull T server) {
        checkNotNull(server, "server");
        register(new ServerResource(server));
        return server;
    }

    void register(Resource resource) {
        resources.add(resource);
    }

    @Override
    public void afterEach(ExtensionContext context) throws Exception {
        stopwatch.reset();
        stopwatch.start();
        for (int i = resources.size() - 1; i >= 0; i--) {
            resources.get(i).cleanUp();
        }

        Throwable firstException = null;
        for (int i = resources.size() - 1; i >= 0; i--) {
            if (firstException != null) {
                resources.get(i).forceCleanUp();
                continue;
            }

            try {
                boolean released = resources.get(i).awaitReleased(
                        timeoutNanos - stopwatch.elapsed(TimeUnit.NANOSECONDS), TimeUnit.NANOSECONDS);
                if (!released) {
                    firstException = new AssertionError(
                            "Resource " + resources.get(i) + " can not be released in time at the end of test");
                }
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                firstException = e;
            }

            if (firstException != null) {
                resources.get(i).forceCleanUp();
            }
        }

        resources.clear();
    }


    interface Resource {
        void cleanUp();
        void forceCleanUp();
        boolean awaitReleased(long duration, TimeUnit timeUnit) throws InterruptedException;
    }

    private static class ManagedChannelResource implements Resource {
        final ManagedChannel channel;

        ManagedChannelResource(ManagedChannel channel) {
            this.channel = channel;
        }

        @Override
        public void cleanUp() {
            channel.shutdown();
        }

        @Override
        public void forceCleanUp() {
            channel.shutdownNow();
        }

        @Override
        public boolean awaitReleased(long duration, TimeUnit timeUnit) throws InterruptedException {
            return channel.awaitTermination(duration, timeUnit);
        }

        @Override
        public String toString() {
            return channel.toString();
        }
    }

    private static class ServerResource implements Resource {
        final Server server;

        ServerResource(Server server) {
            this.server = server;
        }

        @Override
        public void cleanUp() {
            server.shutdown();
        }

        @Override
        public void forceCleanUp() {
            server.shutdownNow();
        }

        @Override
        public boolean awaitReleased(long duration, TimeUnit timeUnit) throws InterruptedException {
            return server.awaitTermination(duration, timeUnit);
        }

        @Override
        public String toString() {
            return server.toString();
        }
    }
}
