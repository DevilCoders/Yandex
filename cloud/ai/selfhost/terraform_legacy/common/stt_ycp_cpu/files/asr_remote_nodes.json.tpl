{
  "WorkerInfo" : {
    "Host" : "${remote_server}",
    "Port" : 443,
    "EnableTls" : true,
    "ChannelsCount" : 16
  },
  "Nodes" : [
    {
      "Id" : {
        "Processor" : {
          "Name" : "local_node"
        },
        "Name" : "acoustic_model"
      },
      "Slots" : [
        {
          "SlotName" : "features",
          "OptimizationTags" : [
            "Tail"
          ],
          "Type" : "NAsrPipelineProto.FBanksTensor"
        },
        {
          "SlotName" : "acoustic_model",
          "SlotType" : "Resource",
          "Type" : "AcousticModel"
        },
        {
          "SlotName" : "logits",
          "SlotType" : "Output",
          "Type" : "NAsrPipelineProto.LogitsTensor"
        }
      ]
    }
  ],
  "Resources" : [
    {
      "Type" : "CtcDecoder",
      "Name" : "general"
    }
  ]
}
