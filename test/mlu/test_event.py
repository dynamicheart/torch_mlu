# pylint: disable=W0612
from __future__ import print_function

import os
import subprocess
import sys
import unittest
import logging
import torch
import torch_mlu
import torch.multiprocessing as mp
from torch.testing._internal.common_utils import NO_MULTIPROCESSING_SPAWN

cur_dir = os.path.dirname(os.path.abspath(__file__))
sys.path.append(cur_dir + "/../")
from common_utils import testinfo, TestCase, run_tests  # pylint: disable=C0413

logging.basicConfig(level=logging.DEBUG)

TEST_MLU_EVENT_IPC = (
    torch.mlu.is_available() and sys.platform != "darwin" and sys.platform != "win32"
)
TEST_MULTIMLU = TEST_MLU_EVENT_IPC and torch.mlu.device_count() > 1


# Multiply by two in a separate stream
def mlu_multiply_two(queue, ready, done):
    ready.set()
    with torch.mlu.stream(torch.mlu.Stream()):
        mlu_event, tensor = queue.get()
        mlu_event.wait()
        tensor.mul_(2)
        mlu_event.record()
        done.set()
        del mlu_event


def _sleep(shape=1):
    tmp = torch.ones((shape, shape)).pin_memory().mlu(non_blocking=True)


class TestEvent(TestCase):
    @testinfo()
    def test_record(self):  # pylint: disable=R0022
        input1 = torch.randn(1, 3, 2, 2).to("mlu")
        output = torch.neg(input1)
        event = torch.mlu.Event()
        event.record()

    @testinfo()
    def test_query(self):
        event = torch.mlu.Event()
        self.assertTrue(event.query())

    @testinfo()
    def test_synchronize_enable_timing(self):
        input1 = torch.randn(1000, 1000, 2, 2).to("mlu")
        input2 = torch.randn(1000, 1000, 2, 2).to("mlu")
        output = torch.neg(input1)
        start = torch.mlu.Event(enable_timing=True)
        end = torch.mlu.Event(enable_timing=True)
        start.record()
        for i in range(10):
            input3 = torch.neg(input1)
            input4 = torch.neg(input2)
        end.record()
        end.synchronize()
        e2e_time_ms = start.elapsed_time(end)
        hardware_time_ms = start.hardware_time(end) / 1000.0
        diff_ms = e2e_time_ms - hardware_time_ms
        self.assertTrue(diff_ms >= 0)

    @testinfo()
    def test_wait(self):
        start = torch.mlu.Event()
        queue = torch.mlu.current_stream()
        user_queue = torch.mlu.Stream()
        _sleep(8000)
        start.record(queue)
        start.wait(user_queue)
        with torch.mlu.stream(user_queue):
            _sleep(200)
        user_queue.synchronize()
        self.assertTrue(start.query())
        self.assertTrue(queue.query())
        self.assertTrue(user_queue.query())

    @testinfo()
    def test_event_repr(self):
        e = torch.mlu.Event()
        self.assertTrue("torch.mlu.Event" in e.__repr__())

    @testinfo()
    def test_generic_event_without_create_context(self):
        if torch.mlu.device_count() < 3:
            return
        torch.mlu.set_device(0)
        a = torch.randn(3, 3).mlu()
        start = torch.mlu.Event(enable_timing=True)
        end = torch.mlu.Event(enable_timing=True)
        start.record()
        a.add(a)
        end.record()
        torch.mlu.set_device(2)
        self.assertTrue(torch_mlu._MLUC._mlu_hasPrimaryContext(0))
        self.assertFalse(torch_mlu._MLUC._mlu_hasPrimaryContext(2))
        elp_time = start.elapsed_time(end)
        self.assertTrue(torch_mlu._MLUC._mlu_hasPrimaryContext(0))
        self.assertFalse(torch_mlu._MLUC._mlu_hasPrimaryContext(2))

    # TODO([PYTORCH-9107]): support torch.mlu._sleep
    # TODO(#884): support mlu multiprocessing
    # @unittest.skipIf(NO_MULTIPROCESSING_SPAWN, "Disabled for environments that \
    #                  don't support multiprocessing with spawn start method")
    # @unittest.skipIf(not TEST_MLU_EVENT_IPC, 'MLU IPC not available')
    # @testinfo()
    # def test_event(self):
    #     ctx = mp.get_context('spawn')
    #     queue = ctx.Queue()
    #     ready = ctx.Event()
    #     done = ctx.Event()
    #     p = ctx.Process(target=mlu_multiply_two, args=(queue, ready, done))
    #     p.start()

    #     ready.wait()
    #     with torch.mlu.stream(torch.mlu.Stream()):
    #         tensor = torch.mlu.FloatTensor([1, 1, 1, 1])
    #         # Use a sleep kernel to test events. Without the event, the
    #         # multiply happens before the add.
    #         event = torch.mlu.Event(interprocess=True)
    #         _sleep(20000)  # about 30 ms
    #         tensor.add_(1)
    #         event.record()
    #         queue.put((event, tensor))
    #         done.wait()  # must wait until subprocess records event
    #         event.synchronize()
    #         self.assertEqual(list(tensor.shape), [3, 3, 3, 3])
    #     p.join()

    # @staticmethod
    # def _test_event_multiprocess_child(event, p2c, c2p):
    #     c2p.put(0)  # notify parent child is ready
    #     p2c.get()  # wait for record in parent
    #     event.synchronize()
    #     c2p.put(1)  # notify parent synchronization is done

    # @unittest.skipIf(NO_MULTIPROCESSING_SPAWN, "Disabled for environments that \
    #                  don't support multiprocessing with spawn start method")
    # @unittest.skipIf(not TEST_MLU_EVENT_IPC, 'MLU IPC not available')
    # @testinfo()
    # def test_event_multiprocess(self):
    #     global _test_event_multiprocess_child
    #     event = torch.mlu.Event(enable_timing=False, interprocess=True)
    #     self.assertTrue(event.query())

    #     ctx = mp.get_context('spawn')
    #     p2c = ctx.SimpleQueue()
    #     c2p = ctx.SimpleQueue()
    #     p = ctx.Process(
    #         target=TestEvent._test_event_multiprocess_child,
    #         args=(event, p2c, c2p))
    #     p.start()

    #     c2p.get()  # wait for until child process is ready
    #     _sleep(20000)  # spin for about 50 ms
    #     event.record()
    #     p2c.put(0)  # notify child event is recorded

    #     self.assertFalse(event.query())
    #     c2p.get()  # wait for synchronization in child
    #     self.assertTrue(event.query())
    #     p.join()

    # TODO: MLU does not support multi device event.
    # @unittest.skipIf(NO_MULTIPROCESSING_SPAWN, "Disabled for environments that \
    #                  don't support multiprocessing with spawn start method")
    # @unittest.skipIf(not TEST_MLU_EVENT_IPC, 'MLU IPC not available')
    # @unittest.skipIf(not TEST_MULTIMLU, 'found only 1 MLU')
    # def test_event_handle_multi_mlu(self):
    #     d0 = torch.device('mlu:0')
    #     d1 = torch.device('mlu:1')
    #     with torch.mlu.device(d0):
    #         e0 = torch.mlu.Event(enable_timing=False, interprocess=True)

    #     with torch.mlu.device(d1):
    #         # create handle on different device from un-recorded event
    #         e0.ipc_handle()

    #     with torch.mlu.device(d0):
    #         e1 = torch.mlu.Event(enable_timing=False, interprocess=True)
    #         stream = torch.mlu.Stream()
    #         _sleep()  # spin for about 50 ms
    #         e1.record(stream)

    #     with torch.mlu.device(d1):
    #         # create handle on different device from recorded event
    #         e1.ipc_handle()

    @staticmethod
    def _test_event_handle_importer_consumer(handle, p2c, c2p):
        e1 = torch.mlu.Event.from_ipc_handle(0, handle)
        c2p.put(0)  # notify parent child is ready
        p2c.get()  # wait for record in parent
        e1.synchronize()
        c2p.put(1)  # nofity synchronization is done in child
        p2c.get(timeout=300)  # wait for parent to finish before destructing child event

    @unittest.skipIf(
        NO_MULTIPROCESSING_SPAWN,
        "Disabled for environments that \
                     don't support multiprocessing with spawn start method",
    )
    @unittest.skipIf(not TEST_MLU_EVENT_IPC, "MLU IPC not available")
    @testinfo()
    def test_event_handle_importer(self):
        e0 = torch.mlu.Event(enable_timing=False, interprocess=True)
        self.assertTrue(e0.query())

        ctx = mp.get_context("spawn")
        # using Queue instead of SimpleQueue to avoid process stuck when calling q.get()
        p2c = ctx.Queue(maxsize=10)
        c2p = ctx.Queue(maxsize=10)
        p = ctx.Process(
            target=TestEvent._test_event_handle_importer_consumer,
            args=(e0.ipc_handle(), p2c, c2p),
        )
        p.start()

        c2p.get()  # wait for child to become ready
        # TODO(CNNLCORE-19088)
        _sleep(50000)  # spin for about 50 ms
        e0.record()
        p2c.put(0)  # notify child event is recorded

        self.assertFalse(e0.query())
        c2p.get()  # wait for synchronization in child
        self.assertTrue(e0.query())
        p2c.put(1)  # notify child that parent is done
        p.join()

    @staticmethod
    def _test_event_handle_exporter_consumer(handle, p2c, c2p):
        stream = torch.mlu.Stream()
        with torch.mlu.stream(stream):
            e1 = torch.mlu.Event.from_ipc_handle(torch.mlu.current_device(), handle)
            # TODO(CNNLCORE-19088)
            _sleep(50000)  # spin for about 50 ms
            e1.record()
            c2p.put(0)
            # wait for parent process finished synchronization before
            # destructing e1
            p2c.get(timeout=300)

    @unittest.skipIf(
        NO_MULTIPROCESSING_SPAWN,
        "Disabled for environments that \
                     don't support multiprocessing with spawn start method",
    )
    @unittest.skipIf(not TEST_MLU_EVENT_IPC, "MLU IPC not available")
    @testinfo()
    def test_event_handle_exporter(self):
        e0 = torch.mlu.Event(enable_timing=False, interprocess=True)

        ctx = mp.get_context("spawn")
        # using Queue instead of SimpleQueue to avoid process stuck when calling q.get()
        p2c = ctx.Queue(maxsize=10)
        c2p = ctx.Queue(maxsize=10)
        p = ctx.Process(
            target=TestEvent._test_event_handle_exporter_consumer,
            args=(e0.ipc_handle(), p2c, c2p),
        )
        p.start()
        # wait for event in child process is recorded
        c2p.get()

        self.assertFalse(e0.query())
        e0.synchronize()
        self.assertTrue(e0.query())
        p2c.put(0)
        p.join()


if __name__ == "__main__":
    unittest.main()
