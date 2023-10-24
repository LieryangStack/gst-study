# 8 Quality Of Service (QoS)

GStreamer中的服务质量（Quality of Service，QoS）涉及测量和调整管道的实时性能。实时性能始终相对于管道时钟来测量，通常在sink同步缓冲区与时钟时发生。

当缓冲区在sink中到达晚了，即它们的运行时间小于时钟的运行时间时，我们称管道存在服务质量问题。可能的原因如下：

- 高CPU负载，没有足够的CPU资源来处理流，导致缓冲区晚到达sink。
- 网络问题。
- 其他资源问题，如磁盘负载、内存瓶颈等。

测量结果将产生QOS事件，旨在调整一个或多个上游元素的数据速率。可以进行两种类型的调整：

 - 基于接收器中的最新观察结果的短期“紧急”修正。
  
 - 基于接收器中观察到的趋势的长期速率修正。
应用程序还可以人为地在同步缓冲区之间引入延迟，这称为限流（throttling）。它可用于限制或减少帧速率，例如。

## 1 Measuring QoS

在同步缓冲区到管道时钟的元素通常会测量当前的QoS。他们还需要保持一些统计数据，以生成QOS事件。

对于到达sink的每个缓冲区，元素需要计算它到达的时间是提前了多少还是延迟了多少。这称为"抖动"（jitter）。负的抖动值意味着缓冲区提前到达，正值意味着缓冲区延迟到达。抖动值表示缓冲区提前或延迟到达的程度。

同步元素还需要计算接收两个连续缓冲区之间经过的时间。我们将其称为"处理时间"，因为这是上游元素生成/处理缓冲区所需的时间。我们可以将这个处理时间与缓冲区的持续时间进行比较，以测量上游元素生成数据的速度，称为"比例"。例如，如果上游可以在1秒钟内生成1秒长的缓冲区，它的运行速度是所需速度的两倍。另一方面，如果它需要2秒才能生成1秒数据的缓冲区，那么上游生成缓冲区的速度太慢，我们将无法保持同步。通常，比例的滚动平均值被保留。

同步元素还需要测量自己的性能，以确定性能问题是否出现在自身的上游。

这些测量数据用于构建QOS事件，该事件发送到上游。请注意，为到达sink的每个缓冲区都会发送一个QOS事件。

## 2 Handling QoS

元素必须在其src pad上安装一个事件函数，以接收QOS事件。通常，元素需要存储QOS事件的值并在数据处理函数中使用它。元素需要使用锁来保护这些QOS值，如下面的示例所示。还要确保将QOS事件传递到上游。

```c
    [...]

    case GST_EVENT_QOS:
    {
      GstQOSType type;
      gdouble proportion;
      GstClockTimeDiff diff;
      GstClockTime timestamp;

      gst_event_parse_qos (event, &type, &proportion, &diff, &timestamp);

      GST_OBJECT_LOCK (decoder);
      priv->qos_proportion = proportion;
      priv->qos_timestamp = timestamp;
      priv->qos_diff = diff;
      GST_OBJECT_UNLOCK (decoder);

      res = gst_pad_push_event (decoder->sinkpad, event);
      break;
    }

    [...]
```

使用QoS值，元素可以进行两种类型的校正：

### 2.1 Short term correction

QOS事件中的时间戳和抖动值可用于进行短期校正。如果抖动为正，表示上一个缓冲到达晚了，我们可以确定具有时间戳小于timestamp + jitter的缓冲区也将到达晚。因此，我们可以丢弃所有时间戳小于timestamp + jitter的缓冲区。

如果知道缓冲的持续时间，下一个可能及时到达的时间戳更好的估计是：timestamp + 2 * jitter + duration。

一个可能的算法通常如下所示：

```c
 [...]

  GST_OBJECT_LOCK (dec);
  qos_proportion = priv->qos_proportion;
  qos_timestamp = priv->qos_timestamp;
  qos_diff = priv->qos_diff;
  GST_OBJECT_UNLOCK (dec);

  /* calculate the earliest valid timestamp */
  if (G_LIKELY (GST_CLOCK_TIME_IS_VALID (qos_timestamp))) {
    if (G_UNLIKELY (qos_diff > 0)) {
      earliest_time = qos_timestamp + 2 * qos_diff + frame_duration;
    } else {
      earliest_time = qos_timestamp + qos_diff;
    }
  } else {
    earliest_time = GST_CLOCK_TIME_NONE;
  }

  /* compare earliest_time to running-time of next buffer */
  if (earliest_time > timestamp)
    goto drop_buffer;

  [...]
```

### 2.2 Long term correction

长期校正稍微复杂一些。它们依赖于QOS事件中的比例字段。元素应通过QoS消息中的比例字段减少它们消耗的资源量。

以下是一些实现这一目标的可能策略：

- 永久删除帧或减小元素的CPU或带宽需求。一些解码器可以跳过解码B帧。

- 切换到较低质量的处理或减小算法复杂性。应注意不要引入令人不悦的视觉或听觉干扰。

- 切换到质量较低的源以减少网络带宽。

- 将更多的CPU周期分配给管道的关键部分。例如，可以通过增加线程优先级来完成这一点。

在所有情况下，当QOS事件中的比例成员接近理想比例1.0时，元素应准备好恢复到其正常处理速率。

## 3 Throttling

应该在同步到时钟的元素中公开一个属性，以配置它们处于节流模式。在节流模式下，缓冲区之间的时间间隔保持在可配置的节流间隔内。这意味着实际上缓冲区速率限制为每个节流间隔1个缓冲区。这可以用于限制帧速率，例如。

当元素配置为节流模式时（通常只在接收端实现），它应该向上游元素生成带有jitter字段设置为节流间隔的QoS事件。这应该指示上游元素跳过或丢弃配置的节流间隔内的剩余缓冲区。

比例字段设置为需要获得所需的节流间隔的所需减速度。实现可以使用QoS节流类型、比例和jitter成员来调整其实现。

默认的接收端基类具有“throttle-time”属性，用于此功能。您可以使用以下命令进行测试：gst-launch-1.0 videotestsrc ! xvimagesink throttle-time=500000000

## 4 QoS Messages

除了在管道中的元素之间发送的QOS事件，还会在管道总线上发布QOS消息，以通知应用程序QOS决策。QOS消息包含了在何时丢弃了某些内容以及丢弃与处理的项目数量。元素必须在以下情况下发布QOS消息：

- 由于QoS原因，该元素丢弃了一个缓冲区。

- 由于QoS原因（质量），元素更改了其处理策略。这可能包括决定丢弃每个B帧以提高处理速度的解码器，或者切换到更低质量算法的效果元素。
