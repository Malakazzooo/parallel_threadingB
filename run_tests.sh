#!/bin/bash

# Compilar
make clean-all && make all
if [ $? -ne 0 ]; then echo "Erro na compilação!"; exit 1; fi

# Configurações
THREADS=(1 2 3 4 5 8 12 20)
FLAGS=("-name" "-size")
DATASET_DIR="Datasets/Dataset-B" 
RESULT_DIR="Result-image-dir"
FULL_PATH="$DATASET_DIR/$RESULT_DIR"

# Execução dos testes
for flag in "${FLAGS[@]}"; do
    for t in "${THREADS[@]}"; do
        echo "------------------------------------------------"
        echo "A processar: $t threads ($flag)"
        
        # --- LIMPEZA REFORÇADA ---
        if [ -d "$FULL_PATH" ]; then
            
            rm -f "$FULL_PATH"/* 2>/dev/null
            
           
            rm -rf "$FULL_PATH"
            
            
            sync 
        fi
        
        
        if [ -d "$FULL_PATH" ]; then
            echo "⚠️ AVISO: Não consegui apagar a pasta de resultados! Tenta fechar visualizadores de imagem."
            
        fi

        # Executar
        ./process-photos-parallel-A "$DATASET_DIR" $t $flag
    done
done

echo ""
echo "✅ Testes concluídos!"