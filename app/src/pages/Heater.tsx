import useConnection from "@/hooks/useConnection";
import useFirmwareEvent from "@/hooks/useFirmwareEvent";
import { zodResolver } from "@hookform/resolvers/zod";
import { IonButton, IonContent, IonInput, IonText } from "@ionic/react";
import { useState } from "react";
import { useForm } from "react-hook-form";
import {
  CartesianGrid,
  Label,
  Legend,
  Line,
  LineChart,
  ReferenceLine,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";
import { z } from "zod";

export default function Heater() {
  const connection = useConnection();

  const [graphData, setGraphData] = useState<GraphPoint[]>([]);
  const [isHeating, setIsHeating] = useState(false);

  const {
    register,
    handleSubmit,
    formState: { errors, isValid },
    getValues,
  } = useForm({
    resolver: zodResolver(HeatingParametersSchema),
    disabled: isHeating,
    mode: "onChange",
  });

  useFirmwareEvent("heating_report", (event) => {
    if (event.finished) setIsHeating(false);
    setGraphData((temperatures) => [
      ...temperatures,
      {
        celsius: event.celsius,
        seconds_elapsed: event.seconds_elapsed,
      },
    ]);
  });

  const onSubmit = async (params: HeatingParameters) => {
    if (!isHeating) {
      setGraphData([]);
      setIsHeating(true);

      await connection.sendCommand({
        heater_control: {
          start: {
            target_celsius: params.target_celsius,
            duration_ms: Math.round(params.duration * 1000),
            p: params.p,
            i: params.i,
            d: params.d,
          },
        },
      });
    } else {
      setIsHeating(false);
      await connection.sendCommand({
        heater_control: {
          stop: {},
        },
      });
    }
  };

  return (
    <IonContent fullscreen>
      <div className="flex h-full w-full flex-col items-center justify-center gap-10 px-20 py-25 lg:flex-row">
        <ResponsiveContainer width="100%" height="100%">
          <LineChart data={graphData} margin={{ bottom: 20, right: 20 }}>
            <CartesianGrid vertical={false} strokeDasharray="3" />
            <ReferenceLine y={getValues("target_celsius")} stroke="white" />
            <XAxis interval={1} dataKey="seconds_elapsed">
              <Label value="Tempo" position="bottom" />
            </XAxis>
            <YAxis domain={[0, 110]} />
            <Tooltip animationDuration={100} />
            <Legend verticalAlign="top" />
            <Line
              dataKey="celsius"
              type="monotone"
              stroke="#8884d8"
              strokeWidth="3px"
              isAnimationActive={false}
              activeDot={{ r: 8 }}
            />
          </LineChart>
        </ResponsiveContainer>
        <form
          onSubmit={handleSubmit(onSubmit)}
          className="flex max-w-1/5 min-w-50 flex-col gap-2"
        >
          <IonInput
            {...register("target_celsius")}
            label="Temperatura target"
            type="number"
            fill="outline"
            labelPlacement="stacked"
          >
            <IonText slot="end">°C</IonText>
          </IonInput>
          {errors.target_celsius?.message && (
            <IonText color="danger">{errors.target_celsius?.message}</IonText>
          )}
          <IonInput
            {...register("duration")}
            label="Duração"
            type="number"
            fill="outline"
            labelPlacement="stacked"
          >
            <IonText slot="end">segundos</IonText>
          </IonInput>
          {errors.duration?.message && (
            <IonText color="danger">{errors.duration?.message}</IonText>
          )}
          <div className="flex gap-2">
            <IonInput
              {...register("p")}
              label="P"
              type="number"
              step="0.1"
              fill="outline"
              labelPlacement="stacked"
            ></IonInput>
            <IonInput
              {...register("i")}
              label="I"
              type="number"
              step="0.1"
              labelPlacement="stacked"
              fill="outline"
            ></IonInput>
            <IonInput
              {...register("d")}
              label="D"
              type="number"
              step="0.1"
              fill="outline"
              labelPlacement="stacked"
            ></IonInput>
          </div>
          {(errors.p || errors.i || errors.d) && (
            <IonText color="danger">PID devem ser preenchidos</IonText>
          )}
          <IonButton
            disabled={!isValid}
            type="submit"
            color={isHeating ? "danger" : "primary"}
          >
            {isHeating ? "Cancelar" : "Aquecer"}
          </IonButton>
        </form>
      </div>
    </IonContent>
  );
}

interface GraphPoint {
  celsius: number;
  seconds_elapsed: number;
}

const coerceNumberNonEmpty = z
  .string()
  .refine((str) => str != "")
  .transform((str) => Number(str));

const HeatingParametersSchema = z.object({
  target_celsius: z.coerce
    .number()
    .min(1, { message: "Temperatura deve ser ao menos 1°C" })
    .max(100, { message: "Temperatura deve ser no máximo 100°C" }),
  duration: z.coerce
    .number()
    .min(1, { message: "Duração deve ser ao menos 1s" })
    .max(60, { message: "Duração deve ser no máximo 60s" }),
  p: coerceNumberNonEmpty,
  i: coerceNumberNonEmpty,
  d: coerceNumberNonEmpty,
});

type HeatingParameters = z.infer<typeof HeatingParametersSchema>;
