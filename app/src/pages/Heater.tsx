import HeaterControlForm, {
  HeatingParameters,
} from "@/components/heater/HeaterControlForm";
import TemperatureChart, {
  GraphPoint,
} from "@/components/heater/TemperatureChart";
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
            p: params.p,
            i: params.i,
            d: params.d,
          },
        },
      });
    } else {
      setIsHeating(false);
      setTargetTemperature(undefined);
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
        </div>
      </div>
    </IonContent>
  );
}
