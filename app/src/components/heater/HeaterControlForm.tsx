import { zodResolver } from "@hookform/resolvers/zod";
import { IonButton, IonInput, IonText } from "@ionic/react";
import { useForm } from "react-hook-form";
import { z } from "zod";

export default function HeaterControlForm({ isHeating, onSubmit }: Props) {
  const {
    register,
    handleSubmit,
    formState: { errors, isValid },
  } = useForm({
    resolver: zodResolver(HeatingParametersSchema),
    disabled: isHeating,
    mode: "onChange",
  });

  return (
    <form
      onSubmit={handleSubmit(onSubmit)}
      className="flex max-w-1/5 min-w-50 flex-col gap-2"
    >
      <IonInput
        {...register("targetTemperature")}
        label="Temperatura target"
        type="number"
        fill="outline"
        labelPlacement="stacked"
      >
        <IonText slot="end">°C</IonText>
      </IonInput>
      {errors.targetTemperature?.message && (
        <IonText color="danger">{errors.targetTemperature?.message}</IonText>
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
  );
}

interface Props {
  isHeating: boolean;
  onSubmit: (params: HeatingParameters) => Promise<void>;
}

const coerceNumberNonEmpty = z
  .string()
  .refine((str) => str != "")
  .transform((str) => Number(str));

const HeatingParametersSchema = z.object({
  targetTemperature: z.coerce
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

export type HeatingParameters = z.infer<typeof HeatingParametersSchema>;
