import HeaterControlForm, {
  HeatingParameters,
} from "@/components/heater/HeaterControlForm";
import TemperatureChart, {
  GraphPoint,
} from "@/components/heater/TemperatureChart";
import { Button } from "@/components/ui/button";
import useBridge from "@/hooks/useBridge";
import useFirmwareEvent from "@/hooks/useFirmwareEvent";
import { IonContent } from "@ionic/react";
import { useState } from "react";

export default function Heater() {
  const bridge = useBridge();

  const [graphData, setGraphData] = useState<GraphPoint[]>([]);
  const [isHeating, setIsHeating] = useState(false);
  const [targetTemperature, setTargetTemperature] = useState<
    number | undefined
  >(undefined);

  useFirmwareEvent("heating_report", (event) => {
    if (event.finished) setIsHeating(false);
    setGraphData((temperatures) => [
      ...temperatures,
      {
        temperature: event.temperature,
        seconds_elapsed: event.seconds_elapsed,
        stage: event.stage,
      },
    ]);
  });

  const onSubmit = async (params: HeatingParameters) => {
    if (!isHeating) {
      setGraphData([]);
      setIsHeating(true);
      setTargetTemperature(params.targetTemperature);

      await bridge.sendCommand({
        heater_control: {
          start: {
            target_temperature: params.targetTemperature,
            duration_ms: Math.round(params.duration * 1000),

            preheat_duration_ms: Math.round(params.preheatDuration * 1000),
            preheat_multiplier: params.preheatMultiplier,

            p: params.p,
            i: params.i,
            d: params.d,
          },
        },
      });
    } else {
      setIsHeating(false);
      await bridge.sendCommand({
        heater_control: {
          stop: {},
        },
      });
    }
  };

  return (
    <IonContent>
      <div className="flex h-full w-full flex-col items-center justify-center gap-10 p-3 md:flex-row">
        <div className="flex-5">
          <TemperatureChart
            graphData={graphData}
            targetTemperature={targetTemperature}
          />
        </div>
        <div className="flex-1">
          <HeaterControlForm isHeating={isHeating} onSubmit={onSubmit} />
          <Button
            onClick={() => {
              setGraphData([]);
              bridge.sendCommand({
                heater_control: {
                  start: {
                    target_temperature: 100,
                    duration_ms: 20000,

                    preheat_duration_ms: 0,
                    preheat_multiplier: 0,

                    p: 0,
                    i: 0,
                    d: 0,
                  },
                },
              });
            }}
          >
            Despejar agua fria
          </Button>
        </div>
      </div>
    </IonContent>
  );
}
